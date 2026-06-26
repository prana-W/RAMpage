#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class Database;
class CommandRegistry;

class PersistenceManager {
   public:
    explicit PersistenceManager(const std::string& filePath);
    ~PersistenceManager();

    void logCommand(const std::string& entry);
    void replay(Database& db, CommandRegistry& registry);

   private:
    std::string             filePath_;
    std::queue<std::string> queue_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::atomic<bool>       shutdown_{false};
    std::thread             flusherThread_;

    // Entry point for the background flusher thread
    void flusherLoop();
};
