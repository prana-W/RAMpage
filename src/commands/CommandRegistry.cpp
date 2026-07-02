#include "CommandRegistry.h"

#include <algorithm>
#include <chrono>
#include <set>

#include "../persistence/PersistenceManager.h"

// handlers is the unordered map used to map the command with the actual function that needs to be
// run
void CommandRegistry::registerCommand(const string& name, CommandHandler fn) {
    handlers[name] = fn;
}

void CommandRegistry::setPersistenceManager(PersistenceManager* pm) {
    pm_ = pm;
}

static vector<string> tokenize(const string& line) {
    vector<string> tokens;
    string current_token;
    bool in_quotes = false;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (isspace(c) && !in_quotes) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token += c;
        }
    }

    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

static string quoteIfNeeded(const string& s) {
    if (s.find(' ') != string::npos)
        return "\"" + s + "\"";
    return s;
}

// Returns current time as milliseconds since Unix epoch
static long long nowMs() {
    return chrono::duration_cast<chrono::milliseconds>(
               chrono::system_clock::now().time_since_epoch())
        .count();
}

void CommandRegistry::logIfWriteCommand(const string& cmdName, const vector<string>& tokens) {
    // Read-only commands — never log these
    static const set<string> WRITE_CMDS = {"SET",   "DEL",  "EXPIRE", "APPEND", "LPUSH",
                                           "RPUSH", "LPOP", "RPOP",   "LSET",   "EXPIRYAT"};

    if (WRITE_CMDS.find(cmdName) == WRITE_CMDS.end())
        return;

    // tokens[0] = command name (original case), tokens[1..] = args

    // ---- EXPIRE key ttlSec  →  EXPIRYAT key epochMs ----
    if (cmdName == "EXPIRE" && tokens.size() >= 3) {
        try {
            long long ttlMs = stoll(tokens[2]) * 1000;
            long long epochMs = nowMs() + ttlMs;
            pm_->logCommand("EXPIRYAT " + quoteIfNeeded(tokens[1]) + " " + to_string(epochMs));
        } catch (...) { /* malformed — skip logging */
        }
        return;
    }

    // ---- SET / LPUSH / RPUSH with optional TTL ----
    // tokens: [CMD, key, val, ttlSec?]
    if ((cmdName == "SET" || cmdName == "LPUSH" || cmdName == "RPUSH") && tokens.size() >= 4) {
        try {
            long long ttlMs = stoll(tokens[3]) * 1000;
            long long epochMs = nowMs() + ttlMs;

            // 1. Log the base command without TTL (creates/updates the key/value)
            pm_->logCommand(cmdName + " " + quoteIfNeeded(tokens[1]) + " " +
                            quoteIfNeeded(tokens[2]));
            // 2. Log the absolute expiry
            pm_->logCommand("EXPIRYAT " + quoteIfNeeded(tokens[1]) + " " + to_string(epochMs));
        } catch (...) { /* malformed TTL — skip logging */
        }
        return;
    }

    string entry = cmdName;  // use uppercased name for consistency
    for (size_t i = 1; i < tokens.size(); ++i) {
        entry += " " + quoteIfNeeded(tokens[i]);
    }
    pm_->logCommand(entry);
}

// --- Token-based execute (called from the RESP server path) ---
CommandResult CommandRegistry::execute(Database& db, int clientFd, const vector<string>& tokens,
                                       PubSubManager* pubSub) {
    if (tokens.empty())
        return {CommandResult::Type::ERROR, "empty command"};

    string cmdName = tokens[0];
    transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper);

    auto it = handlers.find(cmdName);
    if (it == handlers.end())
        return {CommandResult::Type::ERROR, "unknown command '" + tokens[0] + "'"};

    vector<string> args(tokens.begin() + 1, tokens.end());
    CommandContext ctx{db, clientFd, args, pubSub};
    CommandResult result = it->second(ctx);

    if (pm_ &&
        (result.type == CommandResult::Type::SUCCESS || result.type == CommandResult::Type::RESP)) {
        bool shouldLog = (cmdName != "DEL") || (result.message == "1");
        if (shouldLog)
            logIfWriteCommand(cmdName, tokens);
    }

    return result;
}

CommandResult CommandRegistry::execute(Database& db, const string& rawLine, PubSubManager* pubSub) {
    vector<string> tokens = tokenize(rawLine);
    return execute(db, -1, tokens,
                   pubSub);  // delegate to token-based overload, clientFd=-1 for replay
}