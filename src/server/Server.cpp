#include "Server.h"

#include <iostream>
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

static const int MAX_EVENTS = 64;

Server::Server(int p) : port(p), serverFd(-1), epollFd(-1) {
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
        std::cerr << "[server] Failed to create socket\n";
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
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[server] Failed to bind to port " << port << "\n";
        return false;
    }

    if (listen(serverFd, 10) < 0) {
        std::cerr << "[server] Failed to listen\n";
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
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    close(clientFd);
    std::cout << "[server] Client disconnected (fd=" << clientFd << ")\n";
}

void Server::processClientBuffer(int clientFd, std::string& buffer, Database& db,
                                 CommandRegistry& registry) {
    std::string outBuf;  // accumulate all pipelined responses for a single send()

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
        std::string cmdName = cmd.args[0];
        std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper);

        // QUIT / EXIT — send +OK and close gracefully (Redis behavior)
        if (cmdName == "QUIT" || cmdName == "EXIT") {
            outBuf += RESPSerializer::simpleString("OK");
            send(clientFd, outBuf.c_str(), outBuf.size(), 0);
            removeClient(clientFd);
            return;
        }

        // Dispatch through the command registry
        std::string internalResult = registry.execute(db, cmd.args);

        // "RESP:" prefix = pre-serialized RESP bytes, send as-is (used by LRANGE)
        if (internalResult.size() >= 5 && internalResult.compare(0, 5, "RESP:") == 0) {
            outBuf.append(internalResult, 5, std::string::npos);
        } else {
            outBuf += RESPSerializer::serialize(internalResult, cmdName);
        }
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
        std::cerr << "[server] Failed to create epoll instance\n";
        return;
    }

    // Register the listening socket so we know when a new client connects
    if (!addToEpoll(serverFd)) {
        std::cerr << "[server] Failed to add server socket to epoll\n";
        return;
    }

    std::cout << "[server] RAMpage server listening on port " << port << " (RESP protocol) ...\n";

    // Per-client input buffers: accumulates bytes until a full RESP frame arrives
    std::unordered_map<int, std::string> clientBuffers;

    epoll_event events[MAX_EVENTS];

    // --- The single-threaded event loop ---
    while (true) {
        // Block until at least one fd is ready; -1 means wait forever
        int numReady = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numReady < 0) {
            std::cerr << "[server] epoll_wait error\n";
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

                std::cout << "[server] New client connected (fd=" << clientFd << ") from "
                          << inet_ntoa(clientAddr.sin_addr) << "\n";

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
