#pragma once

#include <string>
#include "Status.h"

struct Response {
    Status status;
    std::string message;
    std::string data;
};
