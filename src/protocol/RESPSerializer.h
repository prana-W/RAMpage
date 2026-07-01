#pragma once

#include <string>
#include <vector>

using namespace std;

class RESPSerializer {
   public:
    static string serialize(const string& internalResult, const string& cmdName);

    static string simpleString(const string& s);       // +s\r\n
    static string errorMsg(const string& msg);         // -ERR msg\r\n
    static string integer(long long n);                // :n\r\n
    static string bulkString(const string& s);         // $len\r\ndata\r\n
    static string nullBulkString();                    // $-1\r\n
    static string array(const vector<string>& items);  // *N\r\n...

    // Pub/Sub push messages and acks
    static string pushMessage(const string& type, const string& channel, const string& payload);
    static string subscribeAck(const string& type, const string& channel, int count);
};
