#include "PersistenceManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "../commands/CommandRegistry.h"
#include "../database/Database.h"

PersistenceManager::PersistenceManager(const string& filePath)
    : filePath_(filePath), flusherThread_(&PersistenceManager::flusherLoop, this) {
    cout << "[persistence] AOF log path: " << filePath_ << "\n";
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
        cout << "[persistence] No AOF file found — starting with empty database.\n";
        return;
    }

    cout << "[persistence] Replaying AOF log: " << filePath_ << " ...\n";

    string line;
    int replayed = 0;
    int skipped = 0;

    while (getline(file, line)) {
        if (line.empty())
            continue;

        // Execute command directly; pm_ is nullptr in registry at this point
        // so no re-logging happens
        string result = registry.execute(db, line);
        if (result.substr(0, 4) == "ERR:") {
            cerr << "[persistence] Replay warning — command failed: '" << line << "' → " << result
                 << "\n";
            ++skipped;
        } else {
            ++replayed;
        }
    }

    cout << "[persistence] Replay complete: " << replayed << " commands applied, " << skipped
         << " skipped.\n";
}

void PersistenceManager::flusherLoop() {
    auto parentPath = filesystem::path(filePath_).parent_path();
    if (!parentPath.empty()) {
        error_code ec;
        filesystem::create_directories(parentPath, ec);
        if (ec) {
            cerr << "[persistence] FATAL: cannot create directory '" << parentPath
                 << "': " << ec.message() << "\n";
            return;
        }
    }

    // Open in append mode — creates the file if it does not exist
    ofstream file(filePath_, ios::app);
    if (!file.is_open()) {
        cerr << "[persistence] FATAL: cannot open AOF file for writing: " << filePath_ << "\n";
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
