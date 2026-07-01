#pragma once

#include <chrono>
#include <deque>
#include <string>
#include <unordered_map>
#include <variant>

#include "../utils/Response.h"

using namespace std;

struct DbEntry {
    variant<string, deque<string>> value;
    long long expiryTimeMs = -1;  // -1 means no expiry
};

class Database {
   private:
    unordered_map<string, DbEntry> data;

    // Helper to check and remove expired keys. Returns true if key is valid (not
    // expired/deleted).
    bool checkAndExpire(const string &key);

    // Helper to get current time in ms
    long long getCurrentTimeMs();

   public:
    // Core & Strings
    Response set(const string &key, const string &value, long long ttlMs = -1);
    Response get(const string &key);
    Response del(const string &key);
    Response isExisting(const string &key);
    Response append(const string &key, const string &value);
    Response strlen(const string &key);
    Response ttl(const string &key);
    Response expire(const string &key, long long ttlMs);
    Response expireAt(const string &key, long long epochMs);

    // Lists
    Response lpush(const string &key, const string &value, long long ttlMs = -1);
    Response rpush(const string &key, const string &value, long long ttlMs = -1);
    Response lpop(const string &key);
    Response rpop(const string &key);
    Response llen(const string &key);
    Response lindex(const string &key, long long index);
    Response lset(const string &key, long long index, const string &value);
    Response lrange(const string &key, long long start, long long stop);
};
