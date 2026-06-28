#include "RESPSerializer.h"

#include <sstream>
#include <unordered_set>

static const std::unordered_set<std::string> INTEGER_CMDS = {
    "APPEND", "STRLEN", "LLEN",     "TTL",      "PTTL", "LPUSH", "RPUSH",  "DEL",
    "EXISTS", "EXPIRE", "EXPIREAT", "EXPIRYAT", "INCR", "DECR",  "INCRBY", "DECRBY",
};

static const std::unordered_set<std::string> BULK_CMDS = {
    "GET", "GETSET", "LPOP", "RPOP", "LINDEX",
};

static const std::unordered_set<std::string> ARRAY_CMDS = {
    "LRANGE",
    "KEYS",
    "SMEMBERS",
    "HGETALL",
};

std::string RESPSerializer::simpleString(const std::string& s) {
    return "+" + s + "\r\n";
}

std::string RESPSerializer::errorMsg(const std::string& msg) {
    return "-ERR " + msg + "\r\n";
}

std::string RESPSerializer::integer(long long n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string RESPSerializer::bulkString(const std::string& s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

std::string RESPSerializer::nullBulkString() {
    return "$-1\r\n";
}

std::string RESPSerializer::array(const std::vector<std::string>& items) {
    std::string out = "*" + std::to_string(items.size()) + "\r\n";
    for (const auto& item : items)
        out += bulkString(item);
    return out;
}

static std::vector<std::string> splitPipe(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, '|'))
        result.push_back(token);
    return result;
}

static bool isInteger(const std::string& s) {
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

std::string RESPSerializer::serialize(const std::string& result, const std::string& cmdName) {
    // --- Error response ---
    if (result.size() >= 4 && result.substr(0, 4) == "ERR:") {
        std::string msg = result.substr(4);
        // Special case: WRONGTYPE (Redis convention for type mismatches)
        if (msg.rfind("Wrong type", 0) == 0)
            return "-WRONGTYPE " + msg + "\r\n";
        return errorMsg(msg);
    }

    // --- Success response (must start with "SUCC:") ---
    if (result.size() < 5 || result.substr(0, 5) != "SUCC:") {
        // Malformed internal result — emit as error
        return errorMsg("internal error: " + result);
    }

    std::string payload = result.substr(5);  // everything after "SUCC:"

    // --- Integer commands ---
    if (INTEGER_CMDS.count(cmdName)) {
        if (!isInteger(payload))
            return errorMsg("internal error: expected integer for " + cmdName);
        try {
            return integer(std::stoll(payload));
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
