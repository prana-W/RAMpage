#include "PubSubCommands.h"
#include <algorithm>
#include "../protocol/RESPSerializer.h"
#include "CommandContext.h"

static CommandResult handleSubscribe(const CommandContext& ctx) {
    if (ctx.args.empty())
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'subscribe'"};
    if (ctx.clientFd < 0 || !ctx.pubSub)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    for (const auto& channel : ctx.args) {
        int count = ctx.pubSub->subscribe(ctx.clientFd, channel);
        res += RESPSerializer::subscribeAck("subscribe", channel, count);
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handleUnsubscribe(const CommandContext& ctx) {
    if (ctx.clientFd < 0 || !ctx.pubSub)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    if (ctx.args.empty()) {
        vector<string> unsubbed;
        ctx.pubSub->unsubscribeAll(ctx.clientFd, &unsubbed);
        if (unsubbed.empty()) {
            res += RESPSerializer::subscribeAck("unsubscribe", "", 0);
        } else {
            for (const auto& ch : unsubbed) {
                res += RESPSerializer::subscribeAck("unsubscribe", ch, 0);
            }
        }
    } else {
        for (const auto& channel : ctx.args) {
            int count = ctx.pubSub->unsubscribe(ctx.clientFd, channel);
            res += RESPSerializer::subscribeAck("unsubscribe", channel, count);
        }
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handlePsubscribe(const CommandContext& ctx) {
    if (ctx.args.empty())
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'psubscribe'"};
    if (ctx.clientFd < 0 || !ctx.pubSub)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    for (const auto& pattern : ctx.args) {
        int count = ctx.pubSub->psubscribe(ctx.clientFd, pattern);
        res += RESPSerializer::subscribeAck("psubscribe", pattern, count);
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handlePunsubscribe(const CommandContext& ctx) {
    if (ctx.clientFd < 0 || !ctx.pubSub)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    if (ctx.args.empty()) {
        vector<string> unsubbed;
        ctx.pubSub->punsubscribeAll(ctx.clientFd, &unsubbed);
        if (unsubbed.empty()) {
            res += RESPSerializer::subscribeAck("punsubscribe", "", 0);
        } else {
            for (const auto& pat : unsubbed) {
                res += RESPSerializer::subscribeAck("punsubscribe", pat, 0);
            }
        }
    } else {
        for (const auto& pattern : ctx.args) {
            int count = ctx.pubSub->punsubscribe(ctx.clientFd, pattern);
            res += RESPSerializer::subscribeAck("punsubscribe", pattern, count);
        }
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handlePublish(const CommandContext& ctx) {
    if (ctx.args.size() != 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'publish'"};
    if (!ctx.pubSub)
        return {CommandResult::Type::ERROR, "client context required"};

    int receivers = ctx.pubSub->publish(ctx.args[0], ctx.args[1]);
    return {CommandResult::Type::SUCCESS, to_string(receivers)};
}

static CommandResult handlePubSub(const CommandContext& ctx) {
    if (ctx.args.empty())
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'pubsub'"};
    if (!ctx.pubSub)
        return {CommandResult::Type::ERROR, "client context required"};

    string subcmd = ctx.args[0];
    transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);

    if (subcmd == "CHANNELS") {
        string pattern = ctx.args.size() > 1 ? ctx.args[1] : "";
        auto channels = ctx.pubSub->getActiveChannels(pattern);
        string res = "*";
        res += to_string(channels.size()) + "\r\n";
        for (const auto& ch : channels) {
            res += RESPSerializer::bulkString(ch);
        }
        return {CommandResult::Type::RESP, res};
    } else if (subcmd == "NUMSUB") {
        string res = "*";
        res += to_string((ctx.args.size() - 1) * 2) + "\r\n";
        for (size_t i = 1; i < ctx.args.size(); ++i) {
            res += RESPSerializer::bulkString(ctx.args[i]);
            res += RESPSerializer::integer(ctx.pubSub->getNumSub(ctx.args[i]));
        }
        return {CommandResult::Type::RESP, res};
    } else if (subcmd == "NUMPAT") {
        return {CommandResult::Type::SUCCESS, to_string(ctx.pubSub->getNumPat())};
    }

    return {CommandResult::Type::ERROR,
            "Unknown PUBSUB subcommand or wrong number of arguments for '" + subcmd + "'"};
}

void registerPubSubCommands(CommandRegistry& reg, [[maybe_unused]] PubSubManager& pubSub) {
    reg.registerCommand("SUBSCRIBE", handleSubscribe);
    reg.registerCommand("UNSUBSCRIBE", handleUnsubscribe);
    reg.registerCommand("PSUBSCRIBE", handlePsubscribe);
    reg.registerCommand("PUNSUBSCRIBE", handlePunsubscribe);
    reg.registerCommand("PUBLISH", handlePublish);
    reg.registerCommand("PUBSUB", handlePubSub);
}
