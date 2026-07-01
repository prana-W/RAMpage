#include "PubSubCommands.h"
#include <algorithm>
#include "../protocol/RESPSerializer.h"

static CommandResult handleSubscribe(PubSubManager& pubSub, int clientFd, vector<string>& args) {
    if (args.empty())
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'subscribe'"};
    if (clientFd < 0)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    for (const auto& channel : args) {
        int count = pubSub.subscribe(clientFd, channel);
        res += RESPSerializer::subscribeAck("subscribe", channel, count);
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handleUnsubscribe(PubSubManager& pubSub, int clientFd, vector<string>& args) {
    if (clientFd < 0)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    if (args.empty()) {
        vector<string> unsubbed;
        pubSub.unsubscribeAll(clientFd, &unsubbed);
        if (unsubbed.empty()) {
            res += RESPSerializer::subscribeAck("unsubscribe", "", 0);
        } else {
            for (const auto& ch : unsubbed) {
                res += RESPSerializer::subscribeAck("unsubscribe", ch, 0);
            }
        }
    } else {
        for (const auto& channel : args) {
            int count = pubSub.unsubscribe(clientFd, channel);
            res += RESPSerializer::subscribeAck("unsubscribe", channel, count);
        }
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handlePsubscribe(PubSubManager& pubSub, int clientFd, vector<string>& args) {
    if (args.empty())
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'psubscribe'"};
    if (clientFd < 0)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    for (const auto& pattern : args) {
        int count = pubSub.psubscribe(clientFd, pattern);
        res += RESPSerializer::subscribeAck("psubscribe", pattern, count);
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handlePunsubscribe(PubSubManager& pubSub, int clientFd, vector<string>& args) {
    if (clientFd < 0)
        return {CommandResult::Type::ERROR, "client context required"};

    string res = "";
    if (args.empty()) {
        vector<string> unsubbed;
        pubSub.punsubscribeAll(clientFd, &unsubbed);
        if (unsubbed.empty()) {
            res += RESPSerializer::subscribeAck("punsubscribe", "", 0);
        } else {
            for (const auto& pat : unsubbed) {
                res += RESPSerializer::subscribeAck("punsubscribe", pat, 0);
            }
        }
    } else {
        for (const auto& pattern : args) {
            int count = pubSub.punsubscribe(clientFd, pattern);
            res += RESPSerializer::subscribeAck("punsubscribe", pattern, count);
        }
    }
    return {CommandResult::Type::RESP, res};
}

static CommandResult handlePublish(PubSubManager& pubSub, int clientFd, vector<string>& args) {
    if (args.size() != 2)
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'publish'"};

    int receivers = pubSub.publish(args[0], args[1]);
    return {CommandResult::Type::SUCCESS, to_string(receivers)};
}

static CommandResult handlePubSub(PubSubManager& pubSub, int clientFd, vector<string>& args) {
    if (args.empty())
        return {CommandResult::Type::ERROR, "wrong number of arguments for 'pubsub'"};

    string subcmd = args[0];
    transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);

    if (subcmd == "CHANNELS") {
        string pattern = args.size() > 1 ? args[1] : "";
        auto channels = pubSub.getActiveChannels(pattern);
        string res = "*";
        res += to_string(channels.size()) + "\r\n";
        for (const auto& ch : channels) {
            res += RESPSerializer::bulkString(ch);
        }
        return {CommandResult::Type::RESP, res};
    } else if (subcmd == "NUMSUB") {
        string res = "*";
        res += to_string((args.size() - 1) * 2) + "\r\n";
        for (size_t i = 1; i < args.size(); ++i) {
            res += RESPSerializer::bulkString(args[i]);
            res += RESPSerializer::integer(pubSub.getNumSub(args[i]));
        }
        return {CommandResult::Type::RESP, res};
    } else if (subcmd == "NUMPAT") {
        return {CommandResult::Type::SUCCESS, to_string(pubSub.getNumPat())};
    }

    return {CommandResult::Type::ERROR,
            "Unknown PUBSUB subcommand or wrong number of arguments for '" + subcmd + "'"};
}

void registerPubSubCommands(CommandRegistry& reg, PubSubManager& pubSub) {
    reg.registerCommand("SUBSCRIBE", [&pubSub](Database&, int clientFd, vector<string>& args) {
        return handleSubscribe(pubSub, clientFd, args);
    });
    reg.registerCommand("UNSUBSCRIBE", [&pubSub](Database&, int clientFd, vector<string>& args) {
        return handleUnsubscribe(pubSub, clientFd, args);
    });
    reg.registerCommand("PSUBSCRIBE", [&pubSub](Database&, int clientFd, vector<string>& args) {
        return handlePsubscribe(pubSub, clientFd, args);
    });
    reg.registerCommand("PUNSUBSCRIBE", [&pubSub](Database&, int clientFd, vector<string>& args) {
        return handlePunsubscribe(pubSub, clientFd, args);
    });
    reg.registerCommand("PUBLISH", [&pubSub](Database&, int clientFd, vector<string>& args) {
        return handlePublish(pubSub, clientFd, args);
    });
    reg.registerCommand("PUBSUB", [&pubSub](Database&, int clientFd, vector<string>& args) {
        return handlePubSub(pubSub, clientFd, args);
    });
}
