#include "Database.h"

long long Database::getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool Database::checkAndExpire(const std::string &key) {
    auto it = data.find(key);
    if (it == data.end()) {
        return false;
    }

    if (it->second.expiryTimeMs != -1 && getCurrentTimeMs() >= it->second.expiryTimeMs) {
        data.erase(it);
        return false;
    }
    return true;
}

Response Database::set(const std::string &key, const std::string &value, long long ttlMs) {
    long long expiry = (ttlMs == -1) ? -1 : getCurrentTimeMs() + ttlMs;
    data[key] = {value, expiry};
    return {Status::OK, "Key set successfully", std::monostate{}};
}

Response Database::get(const std::string &key) {
    if (!checkAndExpire(key)) {
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::string>(entry.value)) {
        return {Status::OK, "Key found", std::get<std::string>(entry.value)};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::del(const std::string &key) {
    if (data.erase(key)) {
        return {Status::OK, "Key deleted successfully", std::monostate{}};
    }
    return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};
}

Response Database::isExisting(const std::string &key) {
    if (checkAndExpire(key)) {
        return {Status::OK, "A key-value pair exists!", std::monostate{}};
    }
    return {Status::KEY_NOT_FOUND, "No key-value pair found!", std::monostate{}};
}

Response Database::append(const std::string &key, const std::string &value) {
    if (!checkAndExpire(key)) {
        data[key] = {value, -1};
        return {Status::OK, "Key created and appended", (long long)value.length()};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::string>(entry.value)) {
        auto &str = std::get<std::string>(entry.value);
        str += value;
        return {Status::OK, "String appended", (long long)str.length()};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::strlen(const std::string &key) {
    if (!checkAndExpire(key)) {
        return {Status::OK, "Key not found", (long long)0};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::string>(entry.value)) {
        return {Status::OK, "String length",
                (long long)std::get<std::string>(entry.value).length()};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::ttl(const std::string &key) {
    if (!checkAndExpire(key))
        return {Status::KEY_NOT_FOUND, "Key not found", (long long)-2};  // -2 for missing key

    auto it = data.find(key);
    if (it->second.expiryTimeMs == -1)
        return {Status::OK, "No expiry", (long long)-1};  // -1 for no expiry

    long long timeRemainingMs = it->second.expiryTimeMs - getCurrentTimeMs();
    return {Status::OK, "TTL found", timeRemainingMs / 1000};  // returning TTL in seconds
}

Response Database::expire(const std::string &key, long long ttlMs) {
    if (!checkAndExpire(key))
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};

    data[key].expiryTimeMs = getCurrentTimeMs() + ttlMs;
    return {Status::OK, "Expiry set", std::monostate{}};
}

Response Database::expireAt(const std::string &key, long long epochMs) {
    if (!checkAndExpire(key))
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};

    data[key].expiryTimeMs = epochMs;
    return {Status::OK, "Expiry set (absolute)", std::monostate{}};
}

// Lists
Response Database::lpush(const std::string &key, const std::string &value, long long ttlMs) {
    long long expiry = (ttlMs == -1) ? -1 : getCurrentTimeMs() + ttlMs;
    checkAndExpire(key);  // clear if expired

    if (data.find(key) == data.end()) {
        std::deque<std::string> deq;
        deq.push_front(value);
        data[key] = {deq, expiry};
        return {Status::OK, "List created and pushed", (long long)1};
    }

    // Checks if the key actually holds the data type we are assuming
    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        // Gets the data
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        deq.push_front(value);
        if (ttlMs != -1)
            entry.expiryTimeMs = expiry;
        return {Status::OK, "Pushed to list", (long long)deq.size()};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::rpush(const std::string &key, const std::string &value, long long ttlMs) {
    long long expiry = (ttlMs == -1) ? -1 : getCurrentTimeMs() + ttlMs;
    checkAndExpire(key);  // clear if expired

    if (data.find(key) == data.end()) {
        std::deque<std::string> deq;
        deq.push_back(value);
        data[key] = {deq, expiry};
        return {Status::OK, "List created and pushed", (long long)1};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        deq.push_back(value);
        if (ttlMs != -1)
            entry.expiryTimeMs = expiry;
        return {Status::OK, "Pushed to list", (long long)deq.size()};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::lpop(const std::string &key) {
    if (!checkAndExpire(key)) {
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        if (deq.empty()) {
            return {Status::KEY_NOT_FOUND, "List is empty", std::monostate{}};
        }
        std::string val = deq.front();
        deq.pop_front();
        if (deq.empty())
            data.erase(key);
        return {Status::OK, "Popped from list", val};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::rpop(const std::string &key) {
    if (!checkAndExpire(key)) {
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        if (deq.empty()) {
            return {Status::KEY_NOT_FOUND, "List is empty", std::monostate{}};
        }
        std::string val = deq.back();
        deq.pop_back();
        if (deq.empty())
            data.erase(key);
        return {Status::OK, "Popped from list", val};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::llen(const std::string &key) {
    if (!checkAndExpire(key)) {
        return {Status::OK, "Key not found", (long long)0};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        return {Status::OK, "List length", (long long)deq.size()};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::lindex(const std::string &key, long long index) {
    if (!checkAndExpire(key)) {
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        long long size = deq.size();
        if (index < 0)
            index += size;
        if (index < 0 || index >= size) {
            return {Status::KEY_NOT_FOUND, "Index out of bounds", std::monostate{}};
        }
        return {Status::OK, "Element found", deq[index]};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::lset(const std::string &key, long long index, const std::string &value) {
    if (!checkAndExpire(key)) {
        return {Status::KEY_NOT_FOUND, "Key not found", std::monostate{}};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        long long size = deq.size();
        if (index < 0)
            index += size;
        if (index < 0 || index >= size) {
            return {Status::ERROR, "Index out of bounds", std::monostate{}};
        }
        deq[index] = value;
        return {Status::OK, "Element set", std::monostate{}};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}

Response Database::lrange(const std::string &key, long long start, long long stop) {
    if (!checkAndExpire(key)) {
        return {Status::OK, "Key not found", std::vector<std::string_view>{}};
    }

    auto &entry = data[key];
    if (std::holds_alternative<std::deque<std::string>>(entry.value)) {
        auto &deq = std::get<std::deque<std::string>>(entry.value);
        long long size = deq.size();

        if (start < 0)
            start += size;
        if (stop < 0)
            stop += size;

        if (start < 0)
            start = 0;
        if (stop < 0)
            stop = 0;

        if (start >= size)
            start = size - 1;
        if (stop >= size)
            stop = size - 1;

        std::vector<std::string_view> res;
        if (start <= stop && start < size) {
            res.reserve(stop - start + 1);
            auto it = deq.begin() + start;
            for (long long i = start; i <= stop; ++i, ++it) {
                res.push_back(*it);  // implicit cast to string_view, zero copy
            }
        }
        return {Status::OK, "Range fetched", std::move(res)};
    }
    return {Status::ERROR, "Wrong type operation", std::monostate{}};
}
