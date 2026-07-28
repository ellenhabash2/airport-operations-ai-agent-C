#pragma once
#include <functional>
#include <json/json.h>
#include <string>

class ConversationService {
public:
    struct Dependencies {
        std::function<Json::Value(const std::string &)> create;
        std::function<Json::Value(const std::string &, const std::string &, const std::string &)> save;
        std::function<bool(const std::string &, const std::string &)> owns;
        std::function<Json::Value(const std::string &)> list;
        std::function<Json::Value(const std::string &, const std::string &)> messages;
    };
    ConversationService(); explicit ConversationService(Dependencies dependencies);
    std::string create(const std::string &userId) const;
    Json::Value list(const std::string &userId) const;
    Json::Value loadOwnedMessages(const std::string &conversationId, const std::string &userId) const;
    void requireOwnership(const std::string &conversationId, const std::string &userId) const;
    Json::Value saveUserMessage(const std::string &, const std::string &) const;
    Json::Value saveAssistantMessage(const std::string &, const std::string &) const;
private: Dependencies dependencies_;
};
