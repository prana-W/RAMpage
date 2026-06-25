#include "CommandRegistry.h"
#include <variant>

// Protocol: every handler MUST return either:
//   "SUCC:<payload>"  — success. payload is the value, number, list, or empty for void ops.
//   "ERR:<message>"   — failure. message describes what went wrong.

void registerListCommands(CommandRegistry& reg) {
    reg.registerCommand("LPUSH", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR:wrong number of arguments for 'lpush'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            try { ttlMs = std::stoll(args[2]) * 1000; }
            catch (...) { return "ERR:TTL must be an integer"; }
        }
        Response res = db.lpush(args[0], args[1], ttlMs);
        if (res.status == Status::OK && std::holds_alternative<long long>(res.data)) {
            return "SUCC:" + std::to_string(std::get<long long>(res.data));
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("RPUSH", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR:wrong number of arguments for 'rpush'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            try { ttlMs = std::stoll(args[2]) * 1000; }
            catch (...) { return "ERR:TTL must be an integer"; }
        }
        Response res = db.rpush(args[0], args[1], ttlMs);
        if (res.status == Status::OK && std::holds_alternative<long long>(res.data)) {
            return "SUCC:" + std::to_string(std::get<long long>(res.data));
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("LPOP", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'lpop'";
        Response res = db.lpop(args[0]);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            return "SUCC:" + std::get<std::string>(res.data);
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("RPOP", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'rpop'";
        Response res = db.rpop(args[0]);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            return "SUCC:" + std::get<std::string>(res.data);
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("LLEN", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1) return "ERR:wrong number of arguments for 'llen'";
        Response res = db.llen(args[0]);
        if (std::holds_alternative<long long>(res.data)) {
            return "SUCC:" + std::to_string(std::get<long long>(res.data));
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("LINDEX", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) return "ERR:wrong number of arguments for 'lindex'";
        long long index = 0;
        try { index = std::stoll(args[1]); }
        catch (...) { return "ERR:index must be an integer"; }
        Response res = db.lindex(args[0], index);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            return "SUCC:" + std::get<std::string>(res.data);
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("LSET", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 3) return "ERR:wrong number of arguments for 'lset'";
        long long index = 0;
        try { index = std::stoll(args[1]); }
        catch (...) { return "ERR:index must be an integer"; }
        Response res = db.lset(args[0], index, args[2]);
        if (res.status == Status::OK) return "SUCC:";
        return "ERR:" + res.message;
    });

    reg.registerCommand("LRANGE", [](Database& db, std::vector<std::string>& args) -> std::string {
        if (args.size() < 3) return "ERR:wrong number of arguments for 'lrange'";
        long long start = 0, stop = 0;
        try {
            start = std::stoll(args[1]);
            stop  = std::stoll(args[2]);
        } catch (...) {
            return "ERR:start and stop must be integers";
        }
        Response res = db.lrange(args[0], start, stop);

        if (res.status == Status::OK && std::holds_alternative<std::vector<std::string>>(res.data)) {
            const auto& vec = std::get<std::vector<std::string>>(res.data);
            if (vec.empty()) return "SUCC:";
            // Items joined by '|' so the whole response fits on one TCP line.
            // The SDK splits on '|' to reconstruct the JS array.
            std::string payload = "";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i > 0) payload += "|";
                payload += vec[i];
            }
            return "SUCC:" + payload;
        }
        return "ERR:" + res.message;
    });
}
