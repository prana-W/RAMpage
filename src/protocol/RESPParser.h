#pragma once

#include <string>
#include <vector>

enum class ParseResult { OK, INCOMPLETE, ERROR };

struct ParsedCommand {
    std::vector<std::string> args;  // args[0] = command name, args[1..] = arguments
};

class RESPParser {
   public:
 
    static ParseResult parse(std::string& buf, ParsedCommand& out);

   private:
    static ParseResult parseArray(std::string& buf, ParsedCommand& out);
    static ParseResult parseInline(std::string& buf, ParsedCommand& out);
};
