#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../database/Database.h"

using namespace std;

// Forward declaration — breaks circular include with PersistenceManager
class PersistenceManager;

using CommandHandler = function<string(Database&, int clientFd, vector<string>&)>;

class CommandRegistry {
   private:
    unordered_map<string, CommandHandler> handlers;

    // Nullable — nullptr means "don't log" (used during AOF replay)
    PersistenceManager* pm_ = nullptr;

    void logIfWriteCommand(const string& cmdName, const vector<string>& tokens);

   public:
    void registerCommand(const string& name, CommandHandler fn);

    // Called by the RESP server path — tokens are already parsed.
    // tokens[0] = command name (any case), tokens[1..] = arguments.
    string execute(Database& db, int clientFd, vector<string>& tokens);

    // Called by AOF replay — raw text line is tokenized internally.
    string execute(Database& db, const string& rawLine);

    // Wire up persistence. Call this AFTER replay() so replayed commands
    // are not re-written to the log.
    void setPersistenceManager(PersistenceManager* pm);
};