#include <stdexcept>
#include <variant>
#include "CommandRegistry.h"

static CommandResult handlePing(Database&, int clientFd, vector<string>& args) {
    if (args.empty())
        return {CommandResult::Type::RESP, "+PONG\r\n"};
    return {CommandResult::Type::RESP, "$" + to_string(args[0].size()) + "\r\n" + args[0] + "\r\n"};
}

static CommandResult handleHello(Database&, int clientFd, vector<string>& args) {
    return {CommandResult::Type::RESP,
            "*14\r\n"
            "$6\r\nserver\r\n$7\r\nrampage\r\n"
            "$7\r\nversion\r\n$3\r\n1.0\r\n"
            "$5\r\nproto\r\n:2\r\n"
            "$2\r\nid\r\n:1\r\n"
            "$4\r\nmode\r\n$10\r\nstandalone\r\n"
            "$4\r\nrole\r\n$6\r\nmaster\r\n"
            "$7\r\nmodules\r\n*0\r\n"};
}

static CommandResult handleClient(Database&, int clientFd, vector<string>& args) {
    return {CommandResult::Type::RESP, "+OK\r\n"};
}

static CommandResult handleSet(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'set'"};
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
    Response res = db.set(args[0], args[1], ttlMs);
    if (res.status == Status::OK)
        return {CommandResult::Type::RESP, "+OK\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleGet(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'get'"};
    Response res = db.get(args[0]);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleDel(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'del'"};
    Response res = db.del(args[0]);
    if (res.status == Status::OK)
        return {CommandResult::Type::SUCCESS, "1"};
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::SUCCESS, "0"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleTtl(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'ttl'"};
    Response res = db.ttl(args[0]);
    if (holds_alternative<long long>(res.data))
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleExpire(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'expire'"};
    long long ttlMs = 0;
    try {
        ttlMs = stoll(args[1]) * 1000;
    } catch (const invalid_argument&) {
        return {CommandResult::Type::ERROR, "TTL must be an integer"};
    } catch (const out_of_range&) {
        return {CommandResult::Type::ERROR, "TTL integer out of range"};
    }
    Response res = db.expire(args[0], ttlMs);
    if (res.status == Status::OK)
        return {CommandResult::Type::SUCCESS, "1"};
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::SUCCESS, "0"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleAppend(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'append'"};
    Response res = db.append(args[0], args[1]);
    if (res.status == Status::OK && holds_alternative<long long>(res.data))
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleStrlen(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'strlen'"};
    Response res = db.strlen(args[0]);
    if (holds_alternative<long long>(res.data))
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleExpiryAt(Database& db, int clientFd, vector<string>& args) {
    if (args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'expiryat'"};
    long long epochMs = 0;
    try {
        epochMs = stoll(args[1]);
    } catch (const invalid_argument&) {
        return {CommandResult::Type::ERROR, "epochMs must be an integer"};
    } catch (const out_of_range&) {
        return {CommandResult::Type::ERROR, "epochMs integer out of range"};
    }
    Response res = db.expireAt(args[0], epochMs);
    if (res.status == Status::OK)
        return {CommandResult::Type::SUCCESS, "1"};
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::SUCCESS, "0"};
    return {CommandResult::Type::ERROR, res.message};
}

void registerStringCommands(CommandRegistry& reg) {
    reg.registerCommand("PING", handlePing);
    reg.registerCommand("HELLO", handleHello);
    reg.registerCommand("CLIENT", handleClient);
    reg.registerCommand("SET", handleSet);
    reg.registerCommand("GET", handleGet);
    reg.registerCommand("DEL", handleDel);
    reg.registerCommand("TTL", handleTtl);
    reg.registerCommand("EXPIRE", handleExpire);
    reg.registerCommand("APPEND", handleAppend);
    reg.registerCommand("STRLEN", handleStrlen);
    reg.registerCommand("EXPIRYAT", handleExpiryAt);
}