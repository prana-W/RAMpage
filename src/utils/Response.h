#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Status.h"

using namespace std;

using DataType = variant<monostate, string, vector<string>, long long, vector<string_view>>;

struct Response {
    Status status;
    string message;
    DataType data;
};
