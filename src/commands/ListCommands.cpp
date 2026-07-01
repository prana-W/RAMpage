#include <charconv>
#include <stdexcept>
#include <variant>

#include "CommandRegistry.h"

static CommandResult handleLpush(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lpush'"};
    long long ttlMs = -1;
    if (args.size() >= 3) {
        try {
            ttlMs = stoll(args[2]) * 1000;
        } catch (const invalid_argument&) {
            return {CommandResult::Type::ERROR, "TTL must be an integer"};
        } catch (const out_of_range&) {
            return {CommandResult::Type::ERROR, "TTL integer out of range"};
        }
    }
    Response res = db.lpush(args[0], args[1], ttlMs);
    if (res.status == Status::OK && holds_alternative<long long>(res.data)) {
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    }
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleRpush(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'rpush'"};
    long long ttlMs = -1;
    if (args.size() >= 3) {
        try {
            ttlMs = stoll(args[2]) * 1000;
        } catch (const invalid_argument&) {
            return {CommandResult::Type::ERROR, "TTL must be an integer"};
        } catch (const out_of_range&) {
            return {CommandResult::Type::ERROR, "TTL integer out of range"};
        }
    }
    Response res = db.rpush(args[0], args[1], ttlMs);
    if (res.status == Status::OK && holds_alternative<long long>(res.data)) {
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    }
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLpop(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lpop'"};
    Response res = db.lpop(args[0]);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleRpop(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'rpop'"};
    Response res = db.rpop(args[0]);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLlen(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'llen'"};
    Response res = db.llen(args[0]);
    if (holds_alternative<long long>(res.data)) {
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    }
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLindex(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lindex'"};
    long long index = 0;
    try {
        index = stoll(args[1]);
    } catch (const invalid_argument&) {
        return {CommandResult::Type::ERROR, "index must be an integer"};
    } catch (const out_of_range&) {
        return {CommandResult::Type::ERROR, "index integer out of range"};
    }
    Response res = db.lindex(args[0], index);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLset(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 3)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lset'"};
    long long index = 0;
    try {
        index = stoll(args[1]);
    } catch (const invalid_argument&) {
        return {CommandResult::Type::ERROR, "index must be an integer"};
    } catch (const out_of_range&) {
        return {CommandResult::Type::ERROR, "index integer out of range"};
    }
    Response res = db.lset(args[0], index, args[2]);
    if (res.status == Status::OK)
        return {CommandResult::Type::RESP, "+OK\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLrange(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 3)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lrange'"};
    long long start = 0, stop = 0;
    try {
        start = stoll(args[1]);
        stop = stoll(args[2]);
    } catch (const invalid_argument&) {
        return {CommandResult::Type::ERROR, "start and stop must be integers"};
    } catch (const out_of_range&) {
        return {CommandResult::Type::ERROR, "start and stop integer out of range"};
    }
    Response res = db.lrange(args[0], start, stop);

    if (res.status == Status::OK && holds_alternative<vector<string_view>>(res.data)) {
        const auto& vec = get<vector<string_view>>(res.data);

        size_t totalSize = 16;  // header overhead
        for (const auto& item : vec)
            totalSize += item.size() + 16;  // per-element overhead

        string resp;
        resp.reserve(totalSize);
        resp.append("*");

        char buf[32];
        auto res_size = to_chars(buf, buf + 32, vec.size());
        resp.append(buf, res_size.ptr - buf);
        resp += "\r\n";

        for (const auto& item : vec) {
            resp += '$';
            auto item_size = to_chars(buf, buf + 32, item.size());
            resp.append(buf, item_size.ptr - buf);
            resp += "\r\n";
            resp.append(item.data(), item.size());
            resp += "\r\n";
        }
        return {CommandResult::Type::RESP, resp};
    }
    return {CommandResult::Type::ERROR, res.message};
}

void registerListCommands(CommandRegistry& reg) {
    reg.registerCommand("LPUSH", handleLpush);
    reg.registerCommand("RPUSH", handleRpush);
    reg.registerCommand("LPOP", handleLpop);
    reg.registerCommand("RPOP", handleRpop);
    reg.registerCommand("LLEN", handleLlen);
    reg.registerCommand("LINDEX", handleLindex);
    reg.registerCommand("LSET", handleLset);
    reg.registerCommand("LRANGE", handleLrange);
}
