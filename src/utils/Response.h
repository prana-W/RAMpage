#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Status.h"

using DataType = std::variant<std::monostate, std::string, std::vector<std::string>, long long,
                              std::vector<std::string_view>>;

struct Response {
    Status status;
    std::string message;
    DataType data;
};
