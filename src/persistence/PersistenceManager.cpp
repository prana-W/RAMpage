#include "PersistenceManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "../commands/CommandRegistry.h"
#include "../database/Database.h"
#include "../utils/Logger.h"

PersistenceManager::PersistenceManager(const string& filePath)
    : filePath_(filePath), flusherThread_(&PersistenceManager::flusherLoop, this) {
    Logger::info("persistence", "AOF log path: " + filePath_);
}

PersistenceManager::~PersistenceManager() {
    // Signal the flusher thread to exit, then wait for it to drain the queue
    shutdown_ = true;
    cv_.notify_all();
    if (flusherThread_.joinable())
        flusherThread_.join();
}

void PersistenceManager::logCommand(const string& entry) {
    {
        lock_guard<mutex> lock(mutex_);
        queue_.push(entry);
    }
    cv_.notify_one();  // wake up the flusher thread
}

void PersistenceManager::replay(Database& db, CommandRegistry& registry) {
    ifstream file(filePath_);
    if (!file.is_open()) {
        Logger::info("persistence", "No AOF file found — starting with empty database.");
        return;
    }

    Logger::info("persistence", "Replaying AOF log: " + filePath_ + " ...");

    string line;
    int replayed = 0;
    int skipped = 0;

    while (getline(file, line)) {
        if (line.empty())
            continue;

        // Execute command directly; pm_ is nullptr in registry at this point
        // so no re-logging happens
        CommandResult result = registry.execute(db, line);
        if (result.type == CommandResult::Type::ERROR) {
            Logger::warn("persistence",
                         "Replay warning — command failed: '" + line + "' → " + result.message);
            ++skipped;
        } else {
            ++replayed;
        }
    }

    Logger::info("persistence", "Replay complete: " + to_string(replayed) + " commands applied, " +
                                    to_string(skipped) + " skipped.");
}

void PersistenceManager::flusherLoop() {
    auto parentPath = filesystem::path(filePath_).parent_path();
    if (!parentPath.empty()) {
        error_code ec;
        filesystem::create_directories(parentPath, ec);
        if (ec) {
            Logger::error("persistence", "FATAL: cannot create directory '" + parentPath.string() +
                                             "': " + ec.message());
            return;
        }
    }

    // Open in append mode — creates the file if it does not exist
    ofstream file(filePath_, ios::app);
    if (!file.is_open()) {
        Logger::error("persistence", "FATAL: cannot open AOF file for writing: " + filePath_);
        return;
    }

    while (true) {
        queue<string> localQueue;

        {
            unique_lock<mutex> lock(mutex_);
            // Wait until there is work to do or we are shutting down
            cv_.wait(lock, [this] { return !queue_.empty() || shutdown_.load(); });
            // Drain the shared queue into a local queue under lock (fast swap)
            swap(localQueue, queue_);
        }

        // Write outside the lock — main thread is never blocked on file I/O
        while (!localQueue.empty()) {
            file << localQueue.front() << "\n";
            localQueue.pop();
        }
        file.flush();

        if (shutdown_.load()) {
            // Final drain: catch any entries pushed between our last swap and shutdown
            lock_guard<mutex> lock(mutex_);
            while (!queue_.empty()) {
                file << queue_.front() << "\n";
                queue_.pop();
            }
            file.flush();
            break;
        }
    }
}
