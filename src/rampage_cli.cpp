#include <iostream>
#include <string>

#include "./commands/CommandModules.h"
#include "./commands/CommandRegistry.h"
#include "./database/Database.h"

using namespace std;

// Converts the CommandResult into human-readable CLI output.
string prettyPrint(const CommandResult& resp) {
    if (resp.type == CommandResult::Type::RESP) {
        return resp.message;  // Pass through pre-formatted RESP
    }

    if (resp.type == CommandResult::Type::ERROR) {
        return "(error) " + resp.message;
    }

    string payload = resp.message;
    if (payload.empty())
        return "OK";
    // Numeric payload → show as integer
    bool isNum = !payload.empty() && (isdigit(payload[0]) || payload[0] == '-');
    if (isNum) {
        bool allDigits = true;
        for (size_t i = (payload[0] == '-' ? 1 : 0); i < payload.size(); ++i) {
            if (!isdigit(payload[i])) {
                allDigits = false;
                break;
            }
        }
        if (allDigits)
            return "(integer) " + payload;
    }
    // Pipe-separated list (LRANGE) → show numbered like Redis CLI
    if (payload.find('|') != string::npos) {
        string out;
        int idx = 1;
        size_t pos = 0, found;
        while ((found = payload.find('|', pos)) != string::npos) {
            out += to_string(idx++) + ") \"" + payload.substr(pos, found - pos) + "\"\n";
            pos = found + 1;
        }
        out += to_string(idx) + ") \"" + payload.substr(pos) + "\"";
        return out;
    }
    // Plain string value
    return "\"" + payload + "\"";
}

int main() {
    Database db;
    CommandRegistry registry;

    registerStringCommands(registry);
    registerListCommands(registry);

    cout << "rampage-cli> ";
    string line;
    while (getline(cin, line)) {
        if (line == "exit" || line == "quit")
            break;
        if (!line.empty()) {
            CommandResult result = registry.execute(db, line);
            cout << prettyPrint(result) << "\n";
        }
        cout << "rampage-cli> ";
    }
    return 0;
}
