#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../database/Database.h"

// Forward declaration — breaks circular include with PersistenceManager
class PersistenceManager;

using CommandHandler = std::function<std::string(Database&, int clientFd, std::vector<std::string>&)>;

class CommandRegistry {
   private:
    std::unordered_map<std::string, CommandHandler> handlers;

    // Nullable — nullptr means "don't log" (used during AOF replay)
    PersistenceManager* pm_ = nullptr;

    void logIfWriteCommand(const std::string& cmdName, const std::vector<std::string>& tokens);

   public:
    void registerCommand(const std::string& name, CommandHandler fn);

    // Called by the RESP server path — tokens are already parsed.
    // tokens[0] = command name (any case), tokens[1..] = arguments.
    std::string execute(Database& db, int clientFd, std::vector<std::string>& tokens);

    // Called by AOF replay — raw text line is tokenized internally.
    std::string execute(Database& db, const std::string& rawLine);

    // Wire up persistence. Call this AFTER replay() so replayed commands
    // are not re-written to the log.
    void setPersistenceManager(PersistenceManager* pm);
};