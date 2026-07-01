#include "Server.h"
#include "../utils/Logger.h"

#include <string>
#include <unordered_map>

// POSIX socket and epoll headers (Linux)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "../protocol/RESPParser.h"
#include "../protocol/RESPSerializer.h"

using namespace std;

static const int MAX_EVENTS = 64;

Server::Server(const ServerConfig& cfg, PubSubManager& pubSub)
    : config(cfg), serverFd(-1), epollFd(-1), pubSub_(pubSub) {
}

Server::~Server() {
    if (serverFd != -1)
        close(serverFd);
    if (epollFd != -1)
        close(epollFd);
}

// --- Private Helpers ---

bool Server::setupSocket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        Logger::error("server", "Failed to create socket");
        return false;
    }

    // Allow reuse of the port immediately after the server restarts
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Set socket to non-blocking mode so epoll_wait() doesn't get stuck
    int flags = fcntl(serverFd, F_GETFL, 0);
    fcntl(serverFd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(config.port));

    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("server", "Failed to bind to port " + to_string(config.port));
        return false;
    }

    if (listen(serverFd, 10) < 0) {
        Logger::error("server", "Failed to listen");
        return false;
    }

    return true;
}

bool Server::addToEpoll(int fd) {
    epoll_event ev{};
    ev.events = EPOLLIN;  // notify when fd is ready to read
    ev.data.fd = fd;
    return epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

void Server::removeClient(int clientFd) {
    pubSub_.unsubscribeAll(clientFd);
    pubSub_.punsubscribeAll(clientFd);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    close(clientFd);
    Logger::info("server", "Client disconnected (fd=" + to_string(clientFd) + ")");
}

void Server::processClientBuffer(int clientFd, string& buffer, Database& db,
                                 CommandRegistry& registry) {
    string outBuf;               // accumulate all pipelined responses for a single send()
    outBuf.reserve(128 * 1024);  // Pre-allocate 128KB to avoid reallocation during pipeline batches

    while (true) {
        ParsedCommand cmd;
        ParseResult pr = RESPParser::parse(buffer, cmd);

        if (pr == ParseResult::INCOMPLETE) {
            break;  // wait for more data
        }

        if (pr == ParseResult::ERROR) {
            if (buffer.empty())
                break;
            outBuf += RESPSerializer::errorMsg("Protocol error");
            send(clientFd, outBuf.c_str(), outBuf.size(), 0);
            removeClient(clientFd);
            return;
        }

        // ParseResult::OK — we have a full, parsed command
        if (cmd.args.empty())
            continue;

        // Upper-case the command name for dispatch + serializer lookup
        string cmdName = cmd.args[0];
        transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper);

        // QUIT / EXIT — send +OK and close gracefully (Redis behavior)
        if (cmdName == "QUIT" || cmdName == "EXIT") {
            outBuf += RESPSerializer::simpleString("OK");
            send(clientFd, outBuf.c_str(), outBuf.size(), 0);
            removeClient(clientFd);
            return;
        }

        // Check subscriber mode
        if (pubSub_.isSubscriber(clientFd)) {
            if (cmdName != "SUBSCRIBE" && cmdName != "UNSUBSCRIBE" && cmdName != "PSUBSCRIBE" &&
                cmdName != "PUNSUBSCRIBE" && cmdName != "PING" && cmdName != "QUIT" &&
                cmdName != "EXIT") {
                outBuf += RESPSerializer::errorMsg(
                    "only (P)SUBSCRIBE / (P)UNSUBSCRIBE / PING / QUIT allowed in this context");
                continue;
            }
        }

        // Dispatch through the command registry
        CommandResult result = registry.execute(db, clientFd, cmd.args);

        outBuf += RESPSerializer::serialize(result, cmdName);
    }

    // Flush all accumulated responses in one syscall
    if (!outBuf.empty())
        send(clientFd, outBuf.c_str(), outBuf.size(), 0);
}

// --- Public API ---

void Server::start(Database& db, CommandRegistry& registry) {
    if (!setupSocket())
        return;

    // Create the epoll instance
    epollFd = epoll_create1(0);
    if (epollFd < 0) {
        Logger::error("server", "Failed to create epoll instance");
        return;
    }

    // Register the listening socket so we know when a new client connects
    if (!addToEpoll(serverFd)) {
        Logger::error("server", "Failed to add server socket to epoll");
        return;
    }

    Logger::info("server", "RAMpage server listening on port " + to_string(config.port) +
                               " (RESP protocol) ...");

    // Per-client input buffers: accumulates bytes until a full RESP frame arrives
    unordered_map<int, string> clientBuffers;

    epoll_event events[MAX_EVENTS];

    // --- The single-threaded event loop ---
    while (true) {
        // Block until at least one fd is ready; -1 means wait forever
        int numReady = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numReady < 0) {
            Logger::error("server", "epoll_wait error");
            break;
        }

        for (int i = 0; i < numReady; ++i) {
            int fd = events[i].data.fd;

            if (fd == serverFd) {
                // --- New client connecting ---
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientLen);
                if (clientFd < 0)
                    continue;

                // Make client socket non-blocking too
                int flags = fcntl(clientFd, F_GETFL, 0);
                fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

                addToEpoll(clientFd);
                clientBuffers[clientFd] = "";  // initialize empty buffer for this client

                Logger::info("server", "New client connected (fd=" + to_string(clientFd) +
                                           ") from " + string(inet_ntoa(clientAddr.sin_addr)));

            } else {
                // --- Existing client sent data ---
                char rawBuf[4096];
                ssize_t bytesRead = recv(fd, rawBuf, sizeof(rawBuf) - 1, 0);

                if (bytesRead <= 0) {
                    // Client disconnected or error
                    clientBuffers.erase(fd);
                    removeClient(fd);
                    continue;
                }

                // Append new bytes to this client's personal buffer
                clientBuffers[fd].append(rawBuf, static_cast<size_t>(bytesRead));

                // Process any complete RESP commands in this client's buffer
                processClientBuffer(fd, clientBuffers[fd], db, registry);
            }
        }
    }
}
