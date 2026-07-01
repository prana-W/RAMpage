#pragma once
#include <string>
#include <vector>

using namespace std;

// Forward declarations
class Database;
class PubSubManager;

struct CommandContext {
    Database& db;
    int clientFd;
    const vector<string>& args;
    PubSubManager* pubSub;  // Nullable
};
