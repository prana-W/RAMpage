#pragma once

#include <string>
#include <vector>

class RESPSerializer {
   public:
    static std::string serialize(const std::string& internalResult, const std::string& cmdName);

    static std::string simpleString(const std::string& s);            // +s\r\n
    static std::string errorMsg(const std::string& msg);              // -ERR msg\r\n
    static std::string integer(long long n);                          // :n\r\n
    static std::string bulkString(const std::string& s);              // $len\r\ndata\r\n
    static std::string nullBulkString();                              // $-1\r\n
    static std::string array(const std::vector<std::string>& items);  // *N\r\n...

    // Pub/Sub push messages and acks
    static std::string pushMessage(const std::string& type, const std::string& channel, const std::string& payload);
    static std::string subscribeAck(const std::string& type, const std::string& channel, int count);
};
