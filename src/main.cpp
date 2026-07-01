#include <string>

#include <memory>

#include "./commands/CommandModules.h"
#include "./commands/CommandRegistry.h"
#include "./commands/PubSubCommands.h"
#include "./database/Database.h"
#include "./persistence/PersistenceManager.h"
#include "./pubsub/PubSubManager.h"
#include "./server/Server.h"
#include "./server/ServerConfig.h"
#include "./utils/Logger.h"

using namespace std;

ServerConfig parseArgs(int argc, char* argv[]) {
    ServerConfig config;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            try {
                config.port = stoi(argv[i + 1]);
                i++;
            } catch (...) {
                Logger::error("server", "Invalid port number '" + string(argv[i + 1]) +
                                            "'. Using default: 2006");
                config.port = 2006;
            }
        } else if (arg == "--aof-file" && i + 1 < argc) {
            config.aofPath = argv[i + 1];
            i++;
        } else if (arg == "--persist") {
            config.persistEnabled = true;
        }
    }
    return config;
}

int main(int argc, char* argv[]) {
    ServerConfig config = parseArgs(argc, argv);

    Database db;
    CommandRegistry registry;

    registerStringCommands(registry);
    registerListCommands(registry);

    PubSubManager pubSub;
    registerPubSubCommands(registry, pubSub);

    unique_ptr<PersistenceManager> pm;
    if (config.persistEnabled) {
        pm = make_unique<PersistenceManager>(config.aofPath);
        pm->replay(db, registry);
        registry.setPersistenceManager(pm.get());
    } else {
        Logger::info("persistence", "Persistence disabled (run with --persist to enable)");
    }

    Logger::info("server", "RAMpage starting on port " + to_string(config.port) + " ...");
    Server server(config, pubSub);
    server.start(db, registry);

    return 0;
}