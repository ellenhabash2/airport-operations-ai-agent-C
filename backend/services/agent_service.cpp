#include "agent_service.h"
#include "agent/LLMClient.h"
#include "agent/ToolRegistry.h"
#include "domain_error.h"
#include <memory>

namespace {
AgentService::Runner productionRunner() {
    auto client = std::make_shared<LLMClient>();
    return [client](Json::Value messages) {
        return AgentLoop::run(messages, ToolRegistry::getToolDefinitions(),
            [client](const Json::Value &current, const Json::Value &tools) { return client->chatWithTools(current, tools); },
            [](const std::string &name, const Json::Value &args) { return ToolRegistry::executeTool(name, args); });
    };
}
}
AgentService::AgentService() : AgentService(ConversationService{}, productionRunner()) {}
AgentService::AgentService(ConversationService conversations, Runner runner)
    : conversations_(std::move(conversations)), runner_(std::move(runner)) {}
AgentResult AgentService::query(const std::string &userId, const std::string &query,
                                const std::optional<std::string> &requestedId) const {
    std::string id;
    Json::Value history(Json::arrayValue);
    if (requestedId) { id = *requestedId; history = conversations_.loadOwnedMessages(id, userId); }
    else id = conversations_.create(userId);

    Json::Value messages(Json::arrayValue), system;
    system["role"] = "system";
    system["content"] = "You are AeroMind, an AI assistant for airport operations. Use the provided tools to answer questions about flights, gates, runways, incidents, and weather. You may call multiple tools in sequence if needed. When you have enough information, give a clear, concise final answer.";
    messages.append(system);
    for (const auto &stored : history) {
        const auto role = stored.get("role", "").asString();
        if ((role == "user" || role == "assistant") && stored.isMember("content")) {
            Json::Value item; item["role"] = role; item["content"] = stored["content"]; messages.append(item);
        }
    }
    conversations_.saveUserMessage(id, query);
    Json::Value user; user["role"] = "user"; user["content"] = query; messages.append(user);
    auto loop = runner_(messages);
    if (loop.providerFailed)
        throw DomainError(DomainErrorKind::ProviderUnavailable, "provider_unavailable", "AI provider is currently unavailable");
    auto answer = loop.answer.empty() ? "I couldn't reach a final answer within the allowed number of steps. Please try rephrasing your question." : loop.answer;
    conversations_.saveAssistantMessage(id, answer);
    return {answer, id, loop.toolsUsed};
}
