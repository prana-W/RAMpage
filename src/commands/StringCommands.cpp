#include <variant>

#include "CommandRegistry.h"

void registerStringCommands(CommandRegistry& reg) {
    // PING — baseline health check used by redis-cli and redis-benchmark
    reg.registerCommand("PING", [](Database&, int clientFd, vector<string>& args) -> string {
        // PING [message] — echoes the message, or "PONG" if no argument
        if (args.empty())
            return "RESP:+PONG\r\n";
        return "RESP:$" + to_string(args[0].size()) + "\r\n" + args[0] + "\r\n";
    });

    // HELLO [proto] — Handshake command required by modern Redis clients (e.g. node-redis v4)
    reg.registerCommand("HELLO", [](Database&, int clientFd, vector<string>& args) -> string {
        return "RESP:*14\r\n"
               "$6\r\nserver\r\n$7\r\nrampage\r\n"
               "$7\r\nversion\r\n$3\r\n1.0\r\n"
               "$5\r\nproto\r\n:2\r\n"
               "$2\r\nid\r\n:1\r\n"
               "$4\r\nmode\r\n$10\r\nstandalone\r\n"
               "$4\r\nrole\r\n$6\r\nmaster\r\n"
               "$7\r\nmodules\r\n*0\r\n";
    });

    // CLIENT — Dummy command to satisfy modern clients sending CLIENT SETINFO
    reg.registerCommand("CLIENT", [](Database&, int clientFd, vector<string>& args) -> string {
        return "RESP:+OK\r\n";
    });

    // SET key value [ttlSeconds]
    reg.registerCommand("SET", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'set'";
        long long ttlMs = -1;
        if (args.size() >= 3) {
            try {
                ttlMs = stoll(args[2]) * 1000;
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
    reg.registerCommand("GET", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 1)
            return "ERR:wrong number of arguments for 'get'";
        Response res = db.get(args[0]);
        if (res.status == Status::OK && holds_alternative<string>(res.data)) {
            const string& val = get<string>(res.data);
            return "RESP:$" + to_string(val.size()) + "\r\n" + val + "\r\n";
        }
        // Key not found or wrong type — return nil
        if (res.status == Status::KEY_NOT_FOUND)
            return "RESP:$-1\r\n";
        return "ERR:" + res.message;
    });

    // DEL key → integer: 1 (deleted) or 0 (not found)
    reg.registerCommand("DEL", [](Database& db, int clientFd, vector<string>& args) -> string {
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
    reg.registerCommand("TTL", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 1)
            return "ERR:wrong number of arguments for 'ttl'";
        Response res = db.ttl(args[0]);
        if (holds_alternative<long long>(res.data))
            return "RESP::" + to_string(get<long long>(res.data)) + "\r\n";
        return "ERR:" + res.message;
    });

    // EXPIRE key seconds → integer: 1 (set) or 0 (key not found)
    reg.registerCommand("EXPIRE", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'expire'";
        long long ttlMs = 0;
        try {
            ttlMs = stoll(args[1]) * 1000;
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
    reg.registerCommand("APPEND", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'append'";
        Response res = db.append(args[0], args[1]);
        if (res.status == Status::OK && holds_alternative<long long>(res.data))
            return "RESP::" + to_string(get<long long>(res.data)) + "\r\n";
        return "ERR:" + res.message;
    });

    // STRLEN key → integer (0 if not found)
    reg.registerCommand("STRLEN", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 1)
            return "ERR:wrong number of arguments for 'strlen'";
        Response res = db.strlen(args[0]);
        if (holds_alternative<long long>(res.data))
            return "RESP::" + to_string(get<long long>(res.data)) + "\r\n";
        return "ERR:" + res.message;
    });

    // EXPIRYAT key epochMs — internal AOF command (absolute epoch milliseconds)
    reg.registerCommand("EXPIRYAT", [](Database& db, int clientFd, vector<string>& args) -> string {
        if (args.size() < 2)
            return "ERR:wrong number of arguments for 'expiryat'";
        long long epochMs = 0;
        try {
            epochMs = stoll(args[1]);
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