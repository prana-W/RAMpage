#pragma once

#include <string>

using namespace std;

struct ServerConfig {
    int port = 2006;
    string aofPath = "rampage.rampage";
    bool persistEnabled = false;
};
