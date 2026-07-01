#include "PubSubManager.h"

#include <sys/socket.h>

#include "../protocol/RESPSerializer.h"

using namespace std;

// Basic glob matcher for '*' and '?'
bool PubSubManager::matchPattern(const string& pattern, const string& channel) {
    size_t p = 0, c = 0;
    size_t star_idx = string::npos;
    size_t match_idx = 0;

    while (c < channel.length()) {
        if (p < pattern.length() && (pattern[p] == '?' || pattern[p] == channel[c])) {
            p++;
            c++;
        } else if (p < pattern.length() && pattern[p] == '*') {
            star_idx = p;
            match_idx = c;
            p++;
        } else if (star_idx != string::npos) {
            p = star_idx + 1;
            match_idx++;
            c = match_idx;
        } else {
            return false;
        }
    }
    while (p < pattern.length() && pattern[p] == '*') {
        p++;
    }
    return p == pattern.length();
}

int PubSubManager::subscribe(int clientFd, const string& channel) {
    channelToSubscribers[channel].insert(clientFd);
    clientToChannels[clientFd].insert(channel);
    return static_cast<int>(clientToChannels[clientFd].size() + clientToPatterns[clientFd].size());
}

int PubSubManager::unsubscribe(int clientFd, const string& channel) {
    auto& subs = channelToSubscribers[channel];
    subs.erase(clientFd);
    if (subs.empty()) {
        channelToSubscribers.erase(channel);
    }

    auto it = clientToChannels.find(clientFd);
    if (it != clientToChannels.end()) {
        it->second.erase(channel);
        if (it->second.empty()) {
            clientToChannels.erase(it);
        }
    }
    return static_cast<int>(clientToChannels[clientFd].size() + clientToPatterns[clientFd].size());
}

int PubSubManager::psubscribe(int clientFd, const string& pattern) {
    patternToSubscribers[pattern].insert(clientFd);
    clientToPatterns[clientFd].insert(pattern);
    return static_cast<int>(clientToChannels[clientFd].size() + clientToPatterns[clientFd].size());
}

int PubSubManager::punsubscribe(int clientFd, const string& pattern) {
    auto& subs = patternToSubscribers[pattern];
    subs.erase(clientFd);
    if (subs.empty()) {
        patternToSubscribers.erase(pattern);
    }

    auto it = clientToPatterns.find(clientFd);
    if (it != clientToPatterns.end()) {
        it->second.erase(pattern);
        if (it->second.empty()) {
            clientToPatterns.erase(it);
        }
    }
    return static_cast<int>(clientToChannels[clientFd].size() + clientToPatterns[clientFd].size());
}

int PubSubManager::unsubscribeAll(int clientFd, vector<string>* outChannels) {
    auto it = clientToChannels.find(clientFd);
    if (it == clientToChannels.end())
        return 0;

    int count = 0;
    unordered_set<string> channels = it->second;  // copy
    for (const auto& ch : channels) {
        if (outChannels)
            outChannels->push_back(ch);
        unsubscribe(clientFd, ch);
        count++;
    }
    return count;
}

int PubSubManager::punsubscribeAll(int clientFd, vector<string>* outPatterns) {
    auto it = clientToPatterns.find(clientFd);
    if (it == clientToPatterns.end())
        return 0;

    int count = 0;
    unordered_set<string> patterns = it->second;  // copy
    for (const auto& pat : patterns) {
        if (outPatterns)
            outPatterns->push_back(pat);
        punsubscribe(clientFd, pat);
        count++;
    }
    return count;
}

int PubSubManager::publish(const string& channel, const string& message) {
    unordered_set<int> recipients;

    // Exact matches
    auto it = channelToSubscribers.find(channel);
    if (it != channelToSubscribers.end()) {
        for (int fd : it->second) {
            recipients.insert(fd);
            string msg = RESPSerializer::pushMessage("message", channel, message);
            send(fd, msg.c_str(), msg.size(), 0);
        }
    }

    // Pattern matches
    for (const auto& pair : patternToSubscribers) {
        if (matchPattern(pair.first, channel)) {
            for (int fd : pair.second) {
                // A client could be subscribed via exact channel AND pattern.
                // Redis handles this by sending the message twice, once as 'message', once as
                // 'pmessage'. If we want to emulate Redis perfectly, we don't deduplicate in that
                // sense. We'll send a 'pmessage'.
                string pmsg = "*4\r\n$8\r\npmessage\r\n" + RESPSerializer::bulkString(pair.first) +
                              RESPSerializer::bulkString(channel) +
                              RESPSerializer::bulkString(message);
                send(fd, pmsg.c_str(), pmsg.size(), 0);
                recipients.insert(fd);  // For return count (unique recipients)
            }
        }
    }

    return static_cast<int>(recipients.size());
}

vector<string> PubSubManager::getActiveChannels(const string& pattern) const {
    vector<string> result;
    for (const auto& pair : channelToSubscribers) {
        if (!pair.second.empty()) {
            if (pattern.empty() || matchPattern(pattern, pair.first)) {
                result.push_back(pair.first);
            }
        }
    }
    return result;
}

int PubSubManager::getNumSub(const string& channel) const {
    auto it = channelToSubscribers.find(channel);
    if (it != channelToSubscribers.end()) {
        return static_cast<int>(it->second.size());
    }
    return 0;
}

int PubSubManager::getNumPat() const {
    int total = 0;
    for (const auto& pair : patternToSubscribers) {
        total += static_cast<int>(pair.second.size());
    }
    return total;
}

bool PubSubManager::isSubscriber(int clientFd) const {
    return clientToChannels.count(clientFd) > 0 || clientToPatterns.count(clientFd) > 0;
}
