#include <variant>
#include "../utils/StringUtils.h"
#include "CommandRegistry.h"

static CommandResult handlePing(const CommandContext& ctx) {
    if (ctx.args.empty())
        return {CommandResult::Type::RESP, "+PONG\r\n"};
    return {CommandResult::Type::RESP,
            "$" + to_string(ctx.args[0].size()) + "\r\n" + ctx.args[0] + "\r\n"};
}

static CommandResult handleHello([[maybe_unused]] const CommandContext& ctx) {
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

static CommandResult handleClient([[maybe_unused]] const CommandContext& ctx) {
    return {CommandResult::Type::RESP, "+OK\r\n"};
}

static CommandResult handleSet(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'set'"};
    long long ttlMs = -1;
    if (ctx.args.size() >= 3) {
        auto parsed = StringUtils::parseLong(ctx.args[2]);
        if (!parsed)
            return {CommandResult::Type::ERROR, "TTL must be an integer"};
        ttlMs = *parsed * 1000;
    }
    Response res = ctx.db.set(ctx.args[0], ctx.args[1], ttlMs);
    if (res.status == Status::OK)
        return {CommandResult::Type::RESP, "+OK\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleGet(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'get'"};
    Response res = ctx.db.get(ctx.args[0]);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleDel(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'del'"};
    Response res = ctx.db.del(ctx.args[0]);
    if (res.status == Status::OK)
        return {CommandResult::Type::SUCCESS, "1"};
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::SUCCESS, "0"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleTtl(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'ttl'"};
    Response res = ctx.db.ttl(ctx.args[0]);
    if (holds_alternative<long long>(res.data))
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleExpire(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'expire'"};

    auto parsed = StringUtils::parseLong(ctx.args[1]);
    if (!parsed)
        return {CommandResult::Type::ERROR, "TTL must be an integer"};

    long long ttlMs = *parsed * 1000;
    Response res = ctx.db.expire(ctx.args[0], ttlMs);
    if (res.status == Status::OK)
        return {CommandResult::Type::SUCCESS, "1"};
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::SUCCESS, "0"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleAppend(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'append'"};
    Response res = ctx.db.append(ctx.args[0], ctx.args[1]);
    if (res.status == Status::OK && holds_alternative<long long>(res.data))
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleStrlen(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'strlen'"};
    Response res = ctx.db.strlen(ctx.args[0]);
    if (holds_alternative<long long>(res.data))
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleExpiryAt(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'expiryat'"};

    auto parsed = StringUtils::parseLong(ctx.args[1]);
    if (!parsed)
        return {CommandResult::Type::ERROR, "epochMs must be an integer"};

    long long epochMs = *parsed;
    Response res = ctx.db.expireAt(ctx.args[0], epochMs);
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