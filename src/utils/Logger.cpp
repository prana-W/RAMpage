#include "Logger.h"
#include <iostream>

void Logger::info(const string& component, const string& message) {
    cout << "[" << component << "] " << message << "\n";
}

void Logger::warn(const string& component, const string& message) {
    cerr << "[" << component << "] WARNING: " << message << "\n";
}

void Logger::error(const string& component, const string& message) {
    cerr << "[" << component << "] ERROR: " << message << "\n";
}
