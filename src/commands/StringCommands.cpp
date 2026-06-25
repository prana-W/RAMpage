#include "CommandRegistry.h"
#include <variant>

// Protocol: every handler MUST return either:
//   "SUCC:<payload>"  — success. payload is the value, number, or empty for void ops.
//   "ERR:<message>"   — failure. message describes what went wrong.

void registerStringCommands(CommandRegistry& reg) {
    reg.registerCommand("SET", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR:wrong number of arguments for 'set'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            try { ttlMs = std::stoll(args[2]) * 1000; }
            catch (...) { return "ERR:TTL must be an integer"; }
        }
        Response res = db.set(args[0], args[1], ttlMs);
        if (res.status == Status::OK) return "SUCC:";
        return "ERR:" + res.message;
    });

    reg.registerCommand("GET", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'get'";
        Response res = db.get(args[0]);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            return "SUCC:" + std::get<std::string>(res.data);
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("DEL", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'del'";
        Response res = db.del(args[0]);
        if (res.status == Status::OK) return "SUCC:";
        return "ERR:" + res.message;
    });

    reg.registerCommand("TTL", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'ttl'";
        Response res = db.ttl(args[0]);
        if (std::holds_alternative<long long>(res.data)) {
            return "SUCC:" + std::to_string(std::get<long long>(res.data));
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("EXPIRE", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR:wrong number of arguments for 'expire'";
        long long ttlMs = 0;
        try { ttlMs = std::stoll(args[1]) * 1000; }
        catch (...) { return "ERR:TTL must be an integer"; }
        Response res = db.expire(args[0], ttlMs);
        if (res.status == Status::OK) return "SUCC:";
        return "ERR:" + res.message;
    });

    reg.registerCommand("APPEND", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR:wrong number of arguments for 'append'";
        Response res = db.append(args[0], args[1]);
        if (res.status == Status::OK && std::holds_alternative<long long>(res.data)) {
            return "SUCC:" + std::to_string(std::get<long long>(res.data));
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("STRLEN", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'strlen'";
        Response res = db.strlen(args[0]);
        if (std::holds_alternative<long long>(res.data)) {
            return "SUCC:" + std::to_string(std::get<long long>(res.data));
        }
        return "ERR:" + res.message;
    });
}