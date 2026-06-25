#include <iostream>
#include <string>
#include "./database/Database.h"
#include "./commands/CommandRegistry.h"
#include "./commands/CommandModules.h"
#include "./server/Server.h"

int main(int argc, char* argv[]) {
    int port = 2006;
    
    // Parse arguments for --port
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            try {
                port = std::stoi(argv[i + 1]);
                i++; // skip the port value argument
            } catch (...) {
                std::cerr << "[server] Invalid port number '" << argv[i + 1] << "'. Using default: 2006\n";
                port = 2006;
            }
        }
    }

    Database db;
    CommandRegistry registry;

    registerStringCommands(registry);
    registerListCommands(registry);

    Server server(port);
    server.start(db, registry);

    return 0;
}