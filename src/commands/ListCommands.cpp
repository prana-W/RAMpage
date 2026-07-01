#include <charconv>
#include <variant>
#include "../utils/StringUtils.h"
#include "CommandRegistry.h"

static CommandResult handleLpush(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lpush'"};
    long long ttlMs = -1;
    if (ctx.args.size() >= 3) {
        auto parsed = StringUtils::parseLong(ctx.args[2]);
        if (!parsed)
            return {CommandResult::Type::ERROR, "TTL must be an integer"};
        ttlMs = *parsed * 1000;
    }
    Response res = ctx.db.lpush(ctx.args[0], ctx.args[1], ttlMs);
    if (res.status == Status::OK && holds_alternative<long long>(res.data)) {
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    }
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleRpush(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'rpush'"};
    long long ttlMs = -1;
    if (ctx.args.size() >= 3) {
        auto parsed = StringUtils::parseLong(ctx.args[2]);
        if (!parsed)
            return {CommandResult::Type::ERROR, "TTL must be an integer"};
        ttlMs = *parsed * 1000;
    }
    Response res = ctx.db.rpush(ctx.args[0], ctx.args[1], ttlMs);
    if (res.status == Status::OK && holds_alternative<long long>(res.data)) {
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    }
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLpop(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lpop'"};
    Response res = ctx.db.lpop(ctx.args[0]);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleRpop(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'rpop'"};
    Response res = ctx.db.rpop(ctx.args[0]);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLlen(const CommandContext& ctx) {
    if (ctx.args.size() < 1)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'llen'"};
    Response res = ctx.db.llen(ctx.args[0]);
    if (holds_alternative<long long>(res.data)) {
        return {CommandResult::Type::SUCCESS, to_string(get<long long>(res.data))};
    }
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLindex(const CommandContext& ctx) {
    if (ctx.args.size() < 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lindex'"};

    auto parsed = StringUtils::parseLong(ctx.args[1]);
    if (!parsed)
        return {CommandResult::Type::ERROR, "index must be an integer"};

    long long index = *parsed;
    Response res = ctx.db.lindex(ctx.args[0], index);
    if (res.status == Status::OK && holds_alternative<string>(res.data)) {
        const string& val = get<string>(res.data);
        return {CommandResult::Type::RESP, "$" + to_string(val.size()) + "\r\n" + val + "\r\n"};
    }
    if (res.status == Status::KEY_NOT_FOUND)
        return {CommandResult::Type::RESP, "$-1\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLset(const CommandContext& ctx) {
    if (ctx.args.size() < 3)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lset'"};

    auto parsed = StringUtils::parseLong(ctx.args[1]);
    if (!parsed)
        return {CommandResult::Type::ERROR, "index must be an integer"};

    long long index = *parsed;
    Response res = ctx.db.lset(ctx.args[0], index, ctx.args[2]);
    if (res.status == Status::OK)
        return {CommandResult::Type::RESP, "+OK\r\n"};
    return {CommandResult::Type::ERROR, res.message};
}

static CommandResult handleLrange(const CommandContext& ctx) {
    if (ctx.args.size() < 3)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'lrange'"};

    auto parsedStart = StringUtils::parseLong(ctx.args[1]);
    auto parsedStop = StringUtils::parseLong(ctx.args[2]);

    if (!parsedStart || !parsedStop)
        return {CommandResult::Type::ERROR, "start and stop must be integers"};

    long long start = *parsedStart;
    long long stop = *parsedStop;

    Response res = ctx.db.lrange(ctx.args[0], start, stop);

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
