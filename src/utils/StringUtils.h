#pragma once

#include <optional>
#include <string>

using namespace std;

class StringUtils {
   public:
    static optional<long long> parseLong(const string& str);
};
