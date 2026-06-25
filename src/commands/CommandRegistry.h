#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "../database/Database.h"

//CommandHandler is the name I'll use for: any function that takes a Database& and a vector<string>&, and returns a string
// vector<string> is the tokenized raw line from the user
using CommandHandler = std::function<std::string(Database&, std::vector<std::string>&)>;

class CommandRegistry {
private:
    std::unordered_map<std::string, CommandHandler> handlers;

public:
    void registerCommand(const std::string& name, CommandHandler fn);
    std::string execute(Database& db, const std::string& rawLine);
};