#pragma once
#include "agent/AgentLoop.h"
#include "conversation_service.h"
#include <functional>
#include <json/json.h>
#include <optional>
#include <string>

struct AgentResult { std::string answer; std::string conversationId; Json::Value toolsUsed{Json::arrayValue}; };

class AgentService {
public:
    using Runner = std::function<AgentLoop::Result(Json::Value)>;
    AgentService();
    AgentService(ConversationService conversations, Runner runner);
    AgentResult query(const std::string &userId, const std::string &query,
                      const std::optional<std::string> &conversationId) const;
private:
    ConversationService conversations_;
    Runner runner_;
};
