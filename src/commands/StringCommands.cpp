#include <variant>

#include "CommandRegistry.h"

void registerStringCommands(CommandRegistry& reg) {
    // PING — baseline health check used by redis-cli and redis-benchmark
    reg.registerCommand(
        "PING", [](Database&, int clientFd, std::vector<std::string>& args) -> std::string {
            // PING [message] — echoes the message, or "PONG" if no argument
            if (args.empty())
                return "RESP:+PONG\r\n";
            return "RESP:$" + std::to_string(args[0].size()) + "\r\n" + args[0] + "\r\n";
        });

    // SET key value [ttlSeconds]
    reg.registerCommand(
        "SET", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 2)
                return "ERR:wrong number of arguments for 'set'";
            long long ttlMs = -1;
            if (args.size() >= 3) {
                try {
                    ttlMs = std::stoll(args[2]) * 1000;
                } catch (...) {
                    return "ERR:TTL must be an integer";
                }
            }
            Response res = db.set(args[0], args[1], ttlMs);
            if (res.status == Status::OK)
                return "RESP:+OK\r\n";
            return "ERR:" + res.message;
        });

    // GET key → bulk string or nil ($-1)
    reg.registerCommand(
        "GET", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 1)
                return "ERR:wrong number of arguments for 'get'";
            Response res = db.get(args[0]);
            if (res.status == Status::OK && std::holds_alternative<std::string>(res.data)) {
                const std::string& val = std::get<std::string>(res.data);
                return "RESP:$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
            }
            // Key not found or wrong type — return nil
            if (res.status == Status::KEY_NOT_FOUND)
                return "RESP:$-1\r\n";
            return "ERR:" + res.message;
        });

    // DEL key → integer: 1 (deleted) or 0 (not found)
    reg.registerCommand(
        "DEL", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 1)
                return "ERR:wrong number of arguments for 'del'";
            Response res = db.del(args[0]);
            if (res.status == Status::OK)
                return "RESP::1\r\n";
            if (res.status == Status::KEY_NOT_FOUND)
                return "RESP::0\r\n";
            return "ERR:" + res.message;
        });

    // TTL key → integer seconds (-1 no expiry, -2 not found)
    reg.registerCommand(
        "TTL", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 1)
                return "ERR:wrong number of arguments for 'ttl'";
            Response res = db.ttl(args[0]);
            if (std::holds_alternative<long long>(res.data))
                return "RESP::" + std::to_string(std::get<long long>(res.data)) + "\r\n";
            return "ERR:" + res.message;
        });

    // EXPIRE key seconds → integer: 1 (set) or 0 (key not found)
    reg.registerCommand(
        "EXPIRE", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 2)
                return "ERR:wrong number of arguments for 'expire'";
            long long ttlMs = 0;
            try {
                ttlMs = std::stoll(args[1]) * 1000;
            } catch (...) {
                return "ERR:TTL must be an integer";
            }
            Response res = db.expire(args[0], ttlMs);
            if (res.status == Status::OK)
                return "RESP::1\r\n";
            if (res.status == Status::KEY_NOT_FOUND)
                return "RESP::0\r\n";
            return "ERR:" + res.message;
        });

    // APPEND key value → integer (new string length)
    reg.registerCommand(
        "APPEND", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 2)
                return "ERR:wrong number of arguments for 'append'";
            Response res = db.append(args[0], args[1]);
            if (res.status == Status::OK && std::holds_alternative<long long>(res.data))
                return "RESP::" + std::to_string(std::get<long long>(res.data)) + "\r\n";
            return "ERR:" + res.message;
        });

    // STRLEN key → integer (0 if not found)
    reg.registerCommand(
        "STRLEN", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 1)
                return "ERR:wrong number of arguments for 'strlen'";
            Response res = db.strlen(args[0]);
            if (std::holds_alternative<long long>(res.data))
                return "RESP::" + std::to_string(std::get<long long>(res.data)) + "\r\n";
            return "ERR:" + res.message;
        });

    // EXPIRYAT key epochMs — internal AOF command (absolute epoch milliseconds)
    reg.registerCommand(
        "EXPIRYAT", [](Database& db, int clientFd, std::vector<std::string>& args) -> std::string {
            if (args.size() < 2)
                return "ERR:wrong number of arguments for 'expiryat'";
            long long epochMs = 0;
            try {
                epochMs = std::stoll(args[1]);
            } catch (...) {
                return "ERR:epochMs must be an integer";
            }
            Response res = db.expireAt(args[0], epochMs);
            if (res.status == Status::OK)
                return "RESP::1\r\n";
            if (res.status == Status::KEY_NOT_FOUND)
                return "RESP::0\r\n";
            return "ERR:" + res.message;
        });
}