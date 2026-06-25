#include <chrono>
#include <iostream>
#include <thread>

#include "../src/database/Database.h"

void printVariant(const DataType& data) {
    if (std::holds_alternative<std::string>(data)) {
        std::cout << std::get<std::string>(data);
    } else if (std::holds_alternative<long long>(data)) {
        std::cout << std::get<long long>(data);
    } else if (std::holds_alternative<std::vector<std::string>>(data)) {
        std::cout << "[";
        const auto& vec = std::get<std::vector<std::string>>(data);
        for (size_t i = 0; i < vec.size(); ++i) {
            std::cout << vec[i] << (i + 1 < vec.size() ? ", " : "");
        }
        std::cout << "]";
    } else {
        std::cout << "null";
    }
}

void printResponse(const std::string& prefix, const Response& r) {
    std::cout << prefix << ": " << r.message << " - Data: ";
    printVariant(r.data);
    std::cout << "\n";
}

int main() {
    Database db;

    std::cout << "--- Strings ---\n";
    printResponse("SET", db.set("str1", "hello"));
    printResponse("APPEND", db.append("str1", " world"));
    printResponse("GET", db.get("str1"));
    printResponse("STRLEN", db.strlen("str1"));

    std::cout << "\n--- Lists ---\n";
    printResponse("LPUSH b", db.lpush("list1", "b"));
    printResponse("LPUSH a", db.lpush("list1", "a"));
    printResponse("RPUSH c", db.rpush("list1", "c"));
    printResponse("LRANGE 0 -1", db.lrange("list1", 0, -1));
    printResponse("LPOP", db.lpop("list1"));
    printResponse("LRANGE 0 -1", db.lrange("list1", 0, -1));
    printResponse("RPOP", db.rpop("list1"));
    printResponse("LRANGE 0 -1", db.lrange("list1", 0, -1));

    std::cout << "\n--- TTL Expiry ---\n";
    printResponse("SET (50ms TTL)", db.set("temp", "volatile", 50));
    printResponse("GET (Immediate)", db.get("temp"));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printResponse("GET (After 100ms)", db.get("temp"));

    return 0;
}