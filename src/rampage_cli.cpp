#include <iostream>
#include <string>
#include "./database/Database.h"
#include "./commands/CommandRegistry.h"
#include "./commands/CommandModules.h"

// Converts a raw RESP-flavored response into something readable on screen
std::string prettyPrint(const std::string& resp) {
    if (resp.empty()) return "(nil)";
    char prefix = resp[0];
    std::string body = resp.substr(1);

    switch (prefix) {
        case '+': return body;                 
        case '-': return "(error) " + body;      
        case ':': return "(integer) " + body;  
        case '$': return body == "-1" ? "(nil)" : "\"" + body + "\"";
        default:  return resp;
    }
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
