#pragma once
#include <functional>
#include <json/json.h>
#include <string>
#include <cstddef>

class ConversationService {
public:
    struct Dependencies {
        std::function<Json::Value(const std::string &)> create;
        std::function<Json::Value(const std::string &, const std::string &, const std::string &)> save;
        std::function<bool(const std::string &, const std::string &)> owns;
        std::function<Json::Value(const std::string &)> list;
        std::function<Json::Value(const std::string &, const std::string &)> messages;
        std::function<Json::Value(const std::string &, const std::string &)> createNamed;
        std::function<Json::Value(const std::string &, const std::string &, const std::string &,
            const std::string &, const std::string &, const Json::Value &, const Json::Value &,
            const Json::Value &, const Json::Value &, const Json::Value &)> saveStructured;
        std::function<bool(const std::string &, const std::string &)> remove;
    };
    ConversationService(); explicit ConversationService(Dependencies dependencies);
    std::string create(const std::string &userId) const;
    std::string create(const std::string &userId, const std::string &firstMessage) const;
    Json::Value list(const std::string &userId) const;
    Json::Value loadOwnedMessages(const std::string &conversationId, const std::string &userId) const;
    void requireOwnership(const std::string &conversationId, const std::string &userId) const;
    Json::Value saveUserMessage(const std::string &, const std::string &) const;
    Json::Value saveAssistantMessage(const std::string &, const std::string &) const;
    Json::Value loadReplayHistory(const std::string &, const std::string &, std::size_t maxTurns) const;
    void saveReplayMessage(const std::string &, const std::string &, const std::string &,
                           const std::string &, const std::string &, const Json::Value &,
                           const Json::Value & = Json::Value(), const Json::Value & = Json::Value(),
                           const Json::Value & = Json::Value(), const Json::Value & = Json::Value()) const;
    void deleteConversation(const std::string &, const std::string &) const;
    static std::string titleFromFirstMessage(const std::string &);
    static std::size_t historyMaxTurnsFromEnvironment();
private: Dependencies dependencies_;
};
