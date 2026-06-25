#pragma once

#include "../utils/Response.h"
#include <chrono>
#include <deque>
#include <string>
#include <unordered_map>
#include <variant>

struct DbEntry {
  std::variant<std::string, std::deque<std::string>> value;
  long long expiryTimeMs = -1; // -1 means no expiry
};

class Database {
private:
  std::unordered_map<std::string, DbEntry> data;

  // Helper to check and remove expired keys. Returns true if key is valid (not
  // expired/deleted).
  bool checkAndExpire(const std::string &key);

  // Helper to get current time in ms
  long long getCurrentTimeMs();

public:
  // Core & Strings
  Response set(const std::string &key, const std::string &value,
               long long ttlMs = -1);
  Response get(const std::string &key);
  Response del(const std::string &key);
  Response isExisting(const std::string &key);
  Response append(const std::string &key, const std::string &value);
  Response strlen(const std::string &key);
  Response ttl(const std::string &key);
  Response expire(const std::string &key, long long ttlMs);

  // Lists
  Response lpush(const std::string &key, const std::string &value,
                 long long ttlMs = -1);
  Response rpush(const std::string &key, const std::string &value,
                 long long ttlMs = -1);
  Response lpop(const std::string &key);
  Response rpop(const std::string &key);
  Response llen(const std::string &key);
  Response lindex(const std::string &key, long long index);
  Response lset(const std::string &key, long long index,
                const std::string &value);
  Response lrange(const std::string &key, long long start, long long stop);
};
