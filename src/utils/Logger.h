#pragma once

#include <string>

using namespace std;

class Logger {
   public:
    static void info(const string& component, const string& message);
    static void warn(const string& component, const string& message);
    static void error(const string& component, const string& message);
};
