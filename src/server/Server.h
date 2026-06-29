#pragma once

#include <string>

#include "../commands/CommandRegistry.h"
#include "../database/Database.h"
#include "../pubsub/PubSubManager.h"

class Server {
    int port;
    int serverFd;  // the listening socket file descriptor
    int epollFd;   // the epoll instance file descriptor
    PubSubManager& pubSub_;

    // Sets up the listening socket (socket, bind, listen)
    bool setupSocket();

    // Registers a file descriptor with the epoll instance for read events
    bool addToEpoll(int fd);

    // Removes a file descriptor from the epoll instance and closes it
    void removeClient(int clientFd);

    // Processes buffered data for a client: splits on '\n' and executes each command
    void processClientBuffer(int clientFd, std::string& buffer, Database& db,
                             CommandRegistry& registry);

   public:
    Server(int port, PubSubManager& pubSub);
    ~Server();

    // Starts the single-threaded epoll event loop. Blocks until the process is killed.
    void start(Database& db, CommandRegistry& registry);
};
