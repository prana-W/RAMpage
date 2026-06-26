#include <iostream>
#include <string>

#include "./commands/CommandModules.h"
#include "./commands/CommandRegistry.h"
#include "./database/Database.h"
#include "./persistence/PersistenceManager.h"
#include "./server/Server.h"

int main(int argc, char* argv[]) {
    int         port    = 2006;
    std::string aofPath = "rampage.rampage";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            try {
                port = std::stoi(argv[i + 1]);
                i++;
            } catch (...) {
                std::cerr << "[server] Invalid port number '" << argv[i + 1]
                          << "'. Using default: 2006\n";
                port = 2006;
            }
        } else if (arg == "--aof-file" && i + 1 < argc) {
            aofPath = argv[i + 1];
            i++;
        }
    }

    Database           db;
    CommandRegistry    registry;
    PersistenceManager pm(aofPath);

    registerStringCommands(registry);
    registerListCommands(registry);

    pm.replay(db, registry);

    registry.setPersistenceManager(&pm);

    std::cout << "[server] RAMpage starting on port " << port << " ...\n";
    Server server(port);
    server.start(db, registry);

    return 0;
}