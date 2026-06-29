#include <charconv>
#include <variant>

#include "CommandRegistry.h"

// Protocol: every handler MUST return either:
//   "SUCC:<payload>"  — success. payload is the value, number, list, or empty for void ops.
//   "ERR:<message>"   — failure. message describes what went wrong.

void registerListCommands(CommandRegistry& reg) {
    reg.registerCommand("LPUSH", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'lpush'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            try {
                ttlMs = std::stoll(args[2]) * 1000;
            } catch (...) {
                return "ERR:TTL must be an integer";
            }
        }
        Response res = db.lpush(args[0], args[1], ttlMs);
        if (res.status == Status::OK && std::holds_alternative<long long>(res.data)) {
            return "RESP::" + std::to_string(std::get<long long>(res.data)) + "\r\n";
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("RPUSH", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'rpush'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            try {
                ttlMs = std::stoll(args[2]) * 1000;
            } catch (...) {
                return "ERR:TTL must be an integer";
            }
        }
        Response res = db.rpush(args[0], args[1], ttlMs);
        if (res.status == Status::OK && std::holds_alternative<long long>(res.data)) {
            return "RESP::" + std::to_string(std::get<long long>(res.data)) + "\r\n";
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("LPOP", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1)
            return "ERR:wrong number of arguments for 'lpop'";
        Response res = db.lpop(args[0]);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            const std::string& val = std::get<std::string>(res.data);
            return "RESP:$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        }
        if (res.status == Status::KEY_NOT_FOUND)
            return "RESP:$-1\r\n";  // nil — key/list not found
        return "ERR:" + res.message;
    });

    reg.registerCommand("RPOP", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1)
            return "ERR:wrong number of arguments for 'rpop'";
        Response res = db.rpop(args[0]);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            const std::string& val = std::get<std::string>(res.data);
            return "RESP:$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        }
        if (res.status == Status::KEY_NOT_FOUND)
            return "RESP:$-1\r\n";  // nil — key/list not found
        return "ERR:" + res.message;
    });

    reg.registerCommand("LLEN", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 1)
            return "ERR:wrong number of arguments for 'llen'";
        Response res = db.llen(args[0]);
        if (std::holds_alternative<long long>(res.data)) {
            return "RESP::" + std::to_string(std::get<long long>(res.data)) + "\r\n";
        }
        return "ERR:" + res.message;
    });

    reg.registerCommand("LINDEX", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'lindex'";
        long long index = 0;
        try {
            index = std::stoll(args[1]);
        } catch (...) {
            return "ERR:index must be an integer";
        }
        Response res = db.lindex(args[0], index);
        if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
            const std::string& val = std::get<std::string>(res.data);
            return "RESP:$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        }
        if (res.status == Status::KEY_NOT_FOUND)
            return "RESP:$-1\r\n";  // nil — index out of range or key not found
        return "ERR:" + res.message;
    });

    reg.registerCommand("LSET", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 3)
            return "ERR:wrong number of arguments for 'lset'";
        long long index = 0;
        try {
            index = std::stoll(args[1]);
        } catch (...) {
            return "ERR:index must be an integer";
        }
        Response res = db.lset(args[0], index, args[2]);
        if (res.status == Status::OK)
            return "RESP:+OK\r\n";
        return "ERR:" + res.message;
    });

    reg.registerCommand("LRANGE", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
        if (args.size() < 3)
            return "ERR:wrong number of arguments for 'lrange'";
        long long start = 0, stop = 0;
        try {
            start = std::stoll(args[1]);
            stop = std::stoll(args[2]);
        } catch (...) {
            return "ERR:start and stop must be integers";
        }
        Response res = db.lrange(args[0], start, stop);

        if (res.status == Status::OK &&
            std::holds_alternative<std::vector<std::string_view>>(res.data)) {
            const auto& vec = std::get<std::vector<std::string_view>>(res.data);

            // Build RESP array directly — skip the pipe-join intermediate.
            // Pre-calculate size to do a single allocation.
            size_t totalSize = 16 + 5;  // header overhead + "RESP:" prefix
            for (const auto& item : vec)
                totalSize += item.size() + 16;  // per-element overhead

            std::string resp;
            resp.reserve(totalSize);
            resp.append("RESP:*");

            char buf[32];
            auto res_size = std::to_chars(buf, buf + 32, vec.size());
            resp.append(buf, res_size.ptr - buf);
            resp += "\r\n";

            for (const auto& item : vec) {
                resp += '$';
                auto item_size = std::to_chars(buf, buf + 32, item.size());
                resp.append(buf, item_size.ptr - buf);
                resp += "\r\n";
                resp.append(item.data(), item.size());
                resp += "\r\n";
            }
            return resp;
        }
        return "ERR:" + res.message;
    });
}
