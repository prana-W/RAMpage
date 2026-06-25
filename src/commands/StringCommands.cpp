#include "CommandRegistry.h"
#include <variant>

void registerStringCommands(CommandRegistry& reg) {
    reg.registerCommand("SET", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR wrong number of arguments for 'set'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            ttlMs = std::stoll(args[2]) * 1000;
        }
        Response res = db.set(args[0], args[1], ttlMs);
        return res.message;
    });

    reg.registerCommand("GET", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR wrong number of arguments for 'get'";
        Response res = db.get(args[0]);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            return std::get<std::string>(res.data);
        }
        return res.message; // e.g. "Key not found"
    });

    reg.registerCommand("DEL", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR wrong number of arguments for 'del'";
        Response res = db.del(args[0]);
        return res.message;
    });

    reg.registerCommand("TTL", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR wrong number of arguments for 'ttl'";
        Response res = db.ttl(args[0]);
        if (std::holds_alternative<long long>(res.data)) {
            return std::to_string(std::get<long long>(res.data));
        }
        return res.message;
    });

    reg.registerCommand("EXPIRE", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR wrong number of arguments for 'expire'";
        long long ttlMs = std::stoll(args[1]) * 1000;
        Response res = db.expire(args[0], ttlMs);
        return res.message;
    });

    reg.registerCommand("APPEND", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR wrong number of arguments for 'append'";
        Response res = db.append(args[0], args[1]);
        if (res.status == Status::OK && std::holds_alternative<long long>(res.data)) {
            return std::to_string(std::get<long long>(res.data));
        }
        return res.message;
    });

    reg.registerCommand("STRLEN", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR wrong number of arguments for 'strlen'";
        Response res = db.strlen(args[0]);
        if (std::holds_alternative<long long>(res.data)) {
            return std::to_string(std::get<long long>(res.data));
        }
        return res.message;
    });
}