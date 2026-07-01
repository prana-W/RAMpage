#include "RESPSerializer.h"

#include <sstream>
#include <unordered_set>

static const unordered_set<string> INTEGER_CMDS = {
    "APPEND", "STRLEN", "LLEN",     "TTL",      "PTTL", "LPUSH", "RPUSH",  "DEL",
    "EXISTS", "EXPIRE", "EXPIREAT", "EXPIRYAT", "INCR", "DECR",  "INCRBY", "DECRBY",
};

static const unordered_set<string> BULK_CMDS = {
    "GET", "GETSET", "LPOP", "RPOP", "LINDEX",
};

static const unordered_set<string> ARRAY_CMDS = {
    "LRANGE",
    "KEYS",
    "SMEMBERS",
    "HGETALL",
};

string RESPSerializer::simpleString(const string& s) {
    return "+" + s + "\r\n";
}

string RESPSerializer::errorMsg(const string& msg) {
    return "-ERR " + msg + "\r\n";
}

string RESPSerializer::integer(long long n) {
    return ":" + to_string(n) + "\r\n";
}

string RESPSerializer::bulkString(const string& s) {
    return "$" + to_string(s.size()) + "\r\n" + s + "\r\n";
}

string RESPSerializer::nullBulkString() {
    return "$-1\r\n";
}

string RESPSerializer::array(const vector<string>& items) {
    size_t totalSize = 16;
    for (const auto& item : items)
        totalSize += item.size() + 16;

    string out;
    out.reserve(totalSize);
    out += '*';
    out += to_string(items.size());
    out += "\r\n";
    for (const auto& item : items) {
        out += '$';
        out += to_string(item.size());
        out += "\r\n";
        out += item;
        out += "\r\n";
    }
    return out;
}

static vector<string> splitPipe(const string& s) {
    vector<string> result;
    istringstream ss(s);
    string token;
    while (getline(ss, token, '|'))
        result.push_back(token);
    return result;
}

static bool isInteger(const string& s) {
    if (s.empty())
        return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start == s.size())
        return false;
    for (size_t i = start; i < s.size(); ++i)
        if (s[i] < '0' || s[i] > '9')
            return false;
    return true;
}

string RESPSerializer::pushMessage(const string& type, const string& channel,
                                   const string& payload) {
    string out = "*3\r\n";
    out += bulkString(type);
    out += bulkString(channel);
    out += bulkString(payload);
    return out;
}

string RESPSerializer::subscribeAck(const string& type, const string& channel, int count) {
    string out = "*3\r\n";
    out += bulkString(type);
    out += bulkString(channel);
    out += integer(count);
    return out;
}

string RESPSerializer::serialize(const CommandResult& result, const string& cmdName) {
    if (result.type == CommandResult::Type::RESP) {
        return result.message;
    }

    if (result.type == CommandResult::Type::ERROR) {
        string msg = result.message;
        if (msg.compare(0, 10, "Wrong type") == 0)
            return "-WRONGTYPE " + msg + "\r\n";
        return errorMsg(msg);
    }

    const string& payload = result.message;

    // --- Integer commands ---
    if (INTEGER_CMDS.count(cmdName)) {
        if (!isInteger(payload))
            return errorMsg("internal error: expected integer for " + cmdName);
        try {
            return integer(stoll(payload));
        } catch (...) {
            return errorMsg("internal error: integer overflow");
        }
    }

    // --- Bulk string commands ---
    if (BULK_CMDS.count(cmdName)) {
        if (payload.empty())
            return nullBulkString();  // nil — key not found
        return bulkString(payload);
    }

    // --- Array commands ---
    if (ARRAY_CMDS.count(cmdName)) {
        if (payload.empty())
            return array({});  // empty array
        return array(splitPipe(payload));
    }

    if (payload.empty())
        return simpleString("OK");
    return simpleString(payload);
}
