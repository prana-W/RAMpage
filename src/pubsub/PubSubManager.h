#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PubSubManager {
   private:
    // Exact channels: channel -> set of clientFds
    std::unordered_map<std::string, std::unordered_set<int>> channelToSubscribers;
    std::unordered_map<int, std::unordered_set<std::string>> clientToChannels;

    // Pattern channels: pattern -> set of clientFds
    std::unordered_map<std::string, std::unordered_set<int>> patternToSubscribers;
    std::unordered_map<int, std::unordered_set<std::string>> clientToPatterns;

    // Glob matching helper
    static bool matchPattern(const std::string& pattern, const std::string& channel);

   public:
    int subscribe(int clientFd, const std::string& channel);
    int unsubscribe(int clientFd, const std::string& channel);

    int psubscribe(int clientFd, const std::string& pattern);
    int punsubscribe(int clientFd, const std::string& pattern);

    // Unsubscribes from all channels. Used when client disconnects or bare UNSUBSCRIBE.
    // Returns number of channels unsubscribed from.
    int unsubscribeAll(int clientFd, std::vector<std::string>* outChannels = nullptr);

    // Unsubscribes from all patterns. Used when client disconnects or bare PUNSUBSCRIBE.
    int punsubscribeAll(int clientFd, std::vector<std::string>* outPatterns = nullptr);

    // Publishes a message to all exact channel subscribers and matching pattern subscribers.
    // Returns the total number of clients that received the message.
    int publish(const std::string& channel, const std::string& message);

    // Introspection
    std::vector<std::string> getActiveChannels(const std::string& pattern = "") const;
    int getNumSub(const std::string& channel) const;
    int getNumPat() const;

    bool isSubscriber(int clientFd) const;
};
