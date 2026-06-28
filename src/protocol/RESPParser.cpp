#include "RESPParser.h"

#include <sstream>

static ParseResult parseArray(std::string& buf, ParsedCommand& out) {
    // Find the end of the count line: *<N>\r\n
    size_t firstCRLF = buf.find("\r\n");
    if (firstCRLF == std::string::npos)
        return ParseResult::INCOMPLETE;

    int count = 0;
    try {
        count = std::stoi(buf.substr(1, firstCRLF - 1));
    } catch (...) {
        return ParseResult::ERROR;
    }

    if (count <= 0)
        return ParseResult::ERROR;

    size_t cursor = firstCRLF + 2;  // points past the first \r\n
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        // Each element must be a bulk string: $<len>\r\n<data>\r\n
        if (cursor >= buf.size())
            return ParseResult::INCOMPLETE;

        if (buf[cursor] != '$')
            return ParseResult::ERROR;

        size_t lenEnd = buf.find("\r\n", cursor + 1);
        if (lenEnd == std::string::npos)
            return ParseResult::INCOMPLETE;

        int len = 0;
        try {
            len = std::stoi(buf.substr(cursor + 1, lenEnd - cursor - 1));
        } catch (...) {
            return ParseResult::ERROR;
        }

        if (len < 0) {
            // Null bulk string — treat as empty token
            args.emplace_back("");
            cursor = lenEnd + 2;
            continue;
        }

        size_t dataStart = lenEnd + 2;
        size_t dataEnd = dataStart + static_cast<size_t>(len);

        // Need data + trailing \r\n
        if (dataEnd + 2 > buf.size())
            return ParseResult::INCOMPLETE;

        if (buf[dataEnd] != '\r' || buf[dataEnd + 1] != '\n')
            return ParseResult::ERROR;

        args.push_back(buf.substr(dataStart, static_cast<size_t>(len)));
        cursor = dataEnd + 2;
    }

    // Full command confirmed — consume from buffer
    out.args = std::move(args);
    buf.erase(0, cursor);
    return ParseResult::OK;
}

static ParseResult parseInline(std::string& buf, ParsedCommand& out) {
    size_t nlPos = buf.find('\n');
    if (nlPos == std::string::npos)
        return ParseResult::INCOMPLETE;

    std::string line = buf.substr(0, nlPos);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    // Consume from buffer first (even if the line is empty)
    buf.erase(0, nlPos + 1);

    if (line.empty())
        return ParseResult::ERROR;  // empty line — caller skips it

    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> args;
    while (iss >> token)
        args.push_back(std::move(token));

    if (args.empty())
        return ParseResult::ERROR;

    out.args = std::move(args);
    return ParseResult::OK;
}

ParseResult RESPParser::parse(std::string& buf, ParsedCommand& out) {
    if (buf.empty())
        return ParseResult::INCOMPLETE;

    if (buf[0] == '*')
        return ::parseArray(buf, out);

    return ::parseInline(buf, out);
}
