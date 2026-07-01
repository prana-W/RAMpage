#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

class PubSubManager {
   private:
    // Exact channels: channel -> set of clientFds
    unordered_map<string, unordered_set<int>> channelToSubscribers;
    unordered_map<int, unordered_set<string>> clientToChannels;

    // Pattern channels: pattern -> set of clientFds
    unordered_map<string, unordered_set<int>> patternToSubscribers;
    unordered_map<int, unordered_set<string>> clientToPatterns;

    // Glob matching helper
    static bool matchPattern(const string& pattern, const string& channel);

   public:
    int subscribe(int clientFd, const string& channel);
    int unsubscribe(int clientFd, const string& channel);

    int psubscribe(int clientFd, const string& pattern);
    int punsubscribe(int clientFd, const string& pattern);

    // Unsubscribes from all channels. Used when client disconnects or bare UNSUBSCRIBE.
    // Returns number of channels unsubscribed from.
    int unsubscribeAll(int clientFd, vector<string>* outChannels = nullptr);

    // Unsubscribes from all patterns. Used when client disconnects or bare PUNSUBSCRIBE.
    int punsubscribeAll(int clientFd, vector<string>* outPatterns = nullptr);

    // Publishes a message to all exact channel subscribers and matching pattern subscribers.
    // Returns the total number of clients that received the message.
    int publish(const string& channel, const string& message);

    // Introspection
    vector<string> getActiveChannels(const string& pattern = "") const;
    int getNumSub(const string& channel) const;
    int getNumPat() const;

    bool isSubscriber(int clientFd) const;
};
