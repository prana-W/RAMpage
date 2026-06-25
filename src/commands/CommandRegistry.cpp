#include "CommandRegistry.h"
#include <algorithm>

// handlers is the unordered map used to map the command with the actual function that needs to be run
void CommandRegistry::registerCommand(const std::string &name, CommandHandler fn) {
  handlers[name] = fn;
}

// Splits "SET foo bar" into ["SET", "foo", "bar"]
// Also respects quotes: set foo "Hello World" -> ["set", "foo", "Hello World"]
static std::vector<std::string> tokenize(const std::string &line) {
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

std::string CommandRegistry::execute(Database &db, const std::string &rawLine) {
  std::vector<std::string> tokens = tokenize(rawLine);
  if (tokens.empty()) return "-ERR empty command";

  std::string cmdName = tokens[0];
  std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper); // SET/set/Set all work

  // Check if such a command name exists in our handlers map
  auto it = handlers.find(cmdName);
  if (it == handlers.end()) {
    return "-ERR unknown command '" + tokens[0] + "'";
  }
  
  std::vector<std::string> args(tokens.begin() + 1, tokens.end());
  return it->second(db, args); // call the actual handler
}