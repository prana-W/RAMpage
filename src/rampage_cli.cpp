#include <iostream>
#include <string>
#include "./database/Database.h"
#include "./commands/CommandRegistry.h"
#include "./commands/CommandModules.h"

// Converts the protocol response ("SUCC:..." / "ERR:...") into human-readable CLI output.
std::string prettyPrint(const std::string& resp) {
    if (resp.empty()) return "(nil)";

    // New unified protocol: "SUCC:<payload>" or "ERR:<message>"
    if (resp.substr(0, 5) == "SUCC:") {
        std::string payload = resp.substr(5);
        if (payload.empty()) return "OK";
        // Numeric payload → show as integer
        bool isNum = !payload.empty() && (std::isdigit(payload[0]) || payload[0] == '-');
        if (isNum) {
            bool allDigits = true;
            for (size_t i = (payload[0] == '-' ? 1 : 0); i < payload.size(); ++i) {
                if (!std::isdigit(payload[i])) { allDigits = false; break; }
            }
            if (allDigits) return "(integer) " + payload;
        }
        // Pipe-separated list (LRANGE) → show numbered like Redis CLI
        if (payload.find('|') != std::string::npos) {
            std::string out;
            int idx = 1;
            size_t pos = 0, found;
            while ((found = payload.find('|', pos)) != std::string::npos) {
                out += std::to_string(idx++) + ") \"" + payload.substr(pos, found - pos) + "\"\n";
                pos = found + 1;
            }
            out += std::to_string(idx) + ") \"" + payload.substr(pos) + "\"";
            return out;
        }
        // Plain string value
        return "\"" + payload + "\"";
    }

    if (resp.substr(0, 4) == "ERR:") {
        return "(error) " + resp.substr(4);
    }

    // Fallback for unexpected format
    return resp;
}

int main() {
    Database db;
    CommandRegistry registry;

    registerStringCommands(registry);
    registerListCommands(registry);

    std::cout << "rampage-cli> ";
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "exit" || line == "quit") break;
        if (!line.empty()) {
            std::string result = registry.execute(db, line);
            std::cout << prettyPrint(result) << "\n";
        }
        std::cout << "rampage-cli> ";
    }
    return 0;
}
