#pragma once

#include <string>
#include <variant>
#include <vector>

#include "Status.h"

using DataType = std::variant<std::monostate, std::string, std::vector<std::string>, long long>;

struct Response {
    Status status;
    std::string message;
    DataType data;
};
