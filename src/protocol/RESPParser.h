#pragma once

#include <string>
#include <vector>

using namespace std;

enum class ParseResult { OK, INCOMPLETE, ERROR };

struct ParsedCommand {
    vector<string> args;  // args[0] = command name, args[1..] = arguments
};

class RESPParser {
   public:
    static ParseResult parse(string& buf, ParsedCommand& out);

   private:
    static ParseResult parseArray(string& buf, ParsedCommand& out);
    static ParseResult parseInline(string& buf, ParsedCommand& out);
};
