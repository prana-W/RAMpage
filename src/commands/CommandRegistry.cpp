#include "CommandRegistry.h"

#include <algorithm>
#include <chrono>
#include <set>

#include "../persistence/PersistenceManager.h"

// handlers is the unordered map used to map the command with the actual function that needs to be
// run
void CommandRegistry::registerCommand(const std::string& name, CommandHandler fn) {
    handlers[name] = fn;
}

void CommandRegistry::setPersistenceManager(PersistenceManager* pm) {
    pm_ = pm;
}

static std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool in_quotes = false;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (std::isspace(c) && !in_quotes) {
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

static std::string quoteIfNeeded(const std::string& s) {
    if (s.find(' ') != std::string::npos)
        return "\"" + s + "\"";
    return s;
}

// Returns current time as milliseconds since Unix epoch
static long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void CommandRegistry::logIfWriteCommand(const std::string& cmdName,
                                        const std::vector<std::string>& tokens) {
    // Read-only commands — never log these
    static const std::set<std::string> WRITE_CMDS = {
        "SET", "DEL", "EXPIRE", "APPEND", "LPUSH", "RPUSH", "LPOP", "RPOP", "LSET", "EXPIRYAT"};

    if (WRITE_CMDS.find(cmdName) == WRITE_CMDS.end())
        return;

    // tokens[0] = command name (original case), tokens[1..] = args

    // ---- EXPIRE key ttlSec  →  EXPIRYAT key epochMs ----
    if (cmdName == "EXPIRE" && tokens.size() >= 3) {
        try {
            long long ttlMs = std::stoll(tokens[2]) * 1000;
            long long epochMs = nowMs() + ttlMs;
            pm_->logCommand("EXPIRYAT " + quoteIfNeeded(tokens[1]) + " " + std::to_string(epochMs));
        } catch (...) { /* malformed — skip logging */
        }
        return;
    }

    // ---- SET / LPUSH / RPUSH with optional TTL ----
    // tokens: [CMD, key, val, ttlSec?]
    if ((cmdName == "SET" || cmdName == "LPUSH" || cmdName == "RPUSH") && tokens.size() >= 4) {
        try {
            long long ttlMs = std::stoll(tokens[3]) * 1000;
            long long epochMs = nowMs() + ttlMs;

            // 1. Log the base command without TTL (creates/updates the key/value)
            pm_->logCommand(cmdName + " " + quoteIfNeeded(tokens[1]) + " " +
                            quoteIfNeeded(tokens[2]));
            // 2. Log the absolute expiry
            pm_->logCommand("EXPIRYAT " + quoteIfNeeded(tokens[1]) + " " + std::to_string(epochMs));
        } catch (...) { /* malformed TTL — skip logging */
        }
        return;
    }

    std::string entry = cmdName;  // use uppercased name for consistency
    for (size_t i = 1; i < tokens.size(); ++i) {
        entry += " " + quoteIfNeeded(tokens[i]);
    }
    pm_->logCommand(entry);
}

std::string CommandRegistry::execute(Database& db, const std::string& rawLine) {
    std::vector<std::string> tokens = tokenize(rawLine);
    if (tokens.empty())
        return "ERR:empty command";

    std::string cmdName = tokens[0];
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(),
                   ::toupper);  // SET/set/Set all work

    // Check if such a command name exists in our handlers map
    auto it = handlers.find(cmdName);
    if (it == handlers.end()) {
        return "ERR:unknown command '" + tokens[0] + "'";
    }

    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    std::string result = it->second(db, args);

    // Persist only if: persistence is wired up, command succeeded, and it is a write command
    if (pm_ && result.size() >= 5 && result.substr(0, 5) == "SUCC:") {
        logIfWriteCommand(cmdName, tokens);
    }

    return result;
}