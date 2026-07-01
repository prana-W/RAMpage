#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace std;

class Database;
class CommandRegistry;

class PersistenceManager {
   public:
    explicit PersistenceManager(const string& filePath);
    ~PersistenceManager();

    void logCommand(const string& entry);
    void replay(Database& db, CommandRegistry& registry);

   private:
    string filePath_;
    queue<string> queue_;
    mutex mutex_;
    condition_variable cv_;
    atomic<bool> shutdown_{false};
    thread flusherThread_;

    // Entry point for the background flusher thread
    void flusherLoop();
};
