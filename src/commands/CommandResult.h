#pragma once

#include <string>

using namespace std;

struct CommandResult {
    enum class Type { SUCCESS, ERROR, RESP };
    Type type;
    string message;
};
