#include "agent_service.h"
#include "agent/LLMClient.h"
#include "agent/AgentSafety.h"
#include "agent/PresentationService.h"
#include "agent/ToolRegistry.h"
#include "domain_error.h"
#include <memory>
#include <atomic>
#include <chrono>
#include <map>

namespace {
AgentService::Runner productionRunner() {
    auto client = std::make_shared<LLMClient>();
    return [client](Json::Value messages, const ToolExecutionContext &context) {
        return AgentLoop::run(messages, ToolRegistry::getToolDefinitions(),
            [client](const Json::Value &current, const Json::Value &tools) { return client->chatWithTools(current, tools); },
            [&context](const std::string &name, const Json::Value &args) { return ToolRegistry::executeTool(name, args, context); });
    };
}
std::string newTurnId() {
    static std::atomic<unsigned long long> sequence{0};
    return "turn-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
           "-" + std::to_string(++sequence);
}
Json::Value parseJson(const std::string &text) {
    Json::Value value; Json::CharReaderBuilder builder; std::string error;
    const auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    if (reader->parse(text.data(), text.data() + text.size(), &value, &error)) return value;
    return Json::Value();
}
}
AgentService::AgentService() : AgentService(ConversationService{}, productionRunner()) {}
AgentService::AgentService(ConversationService conversations, Runner runner)
    : conversations_(std::move(conversations)), runner_(std::move(runner)) {}
AgentResult AgentService::query(const std::string &userId, const std::string &query,
                                const std::optional<std::string> &requestedId) const {
    std::string id;
    Json::Value history(Json::arrayValue);
    if (requestedId) { id = *requestedId; history = conversations_.loadReplayHistory(id, userId, ConversationService::historyMaxTurnsFromEnvironment()); }
    else id = conversations_.create(userId, query);

    Json::Value messages(Json::arrayValue), system;
    system["role"] = "system";
    system["content"] = "You are AeroMind, an AI assistant for airport operations. Use the provided tools to answer questions about flights, gates, runways, incidents, and weather. You may call multiple tools in sequence if needed. When you have enough information, give a clear, concise final answer.";
    messages.append(system);
    for (const auto &stored : history) messages.append(stored);
    const auto turnId = newTurnId();
    Json::Value user; user["role"] = "user"; user["content"] = query; messages.append(user);
    Json::Value userMeta; userMeta["schema_version"] = 1;
    conversations_.saveReplayMessage(id, "user", query, turnId, "in_progress", user,
                                     Json::Value(), Json::Value(), Json::Value(), userMeta);
    ToolExecutionContext context;
    context.authenticated = true;
    context.userId = userId;
    context.conversationId = id;
    auto loop = runner_(messages, context);
    const Json::Value publicExecutions = AgentSafety::toolExecutionsJson(loop.toolExecutions);
    Json::Value presentation;
    if (const auto generated = PresentationService::generate(loop.toolExecutions))
        presentation = *generated;
    std::map<std::string, std::string> toolNames;
    for (Json::ArrayIndex index = 0; index < loop.generatedMessages.size(); ++index) {
        const auto &generated = loop.generatedMessages[index];
        const auto role = generated.get("role", "assistant").asString();
        const auto content = generated.get("content", "").asString();
        const bool finalMessage = role == "assistant" && !generated.isMember("tool_calls") &&
                                  index + 1 == loop.generatedMessages.size() && !loop.providerFailed;
        Json::Value metadata; metadata["schema_version"] = 1; metadata["execution_order"] = index;
        Json::Value calls, results;
        if (role == "assistant" && generated["tool_calls"].isArray()) {
            calls = generated["tool_calls"];
            for (const auto &call : calls)
                toolNames[call.get("id", "").asString()] = call["function"].get("name", "").asString();
        }
        if (role == "tool") {
            results = parseJson(content);
            metadata["tool_call_id"] = generated.get("tool_call_id", "");
            metadata["tool_name"] = toolNames[metadata["tool_call_id"].asString()];
        }
        if (finalMessage) {
            metadata["tools_used"] = loop.toolsUsed;
            metadata["tool_executions"] = publicExecutions;
        }
        conversations_.saveReplayMessage(id, role, content, turnId,
            finalMessage ? "completed" : (loop.providerFailed ? "failed" : "in_progress"),
            generated, calls, results, finalMessage ? presentation : Json::Value(), metadata);
    }
    if (loop.providerFailed)
        throw DomainError(DomainErrorKind::ProviderUnavailable, "provider_unavailable", "AI provider is currently unavailable");
    auto answer = loop.answer.empty() ? "I couldn't reach a final answer within the allowed number of steps. Please try rephrasing your question." : loop.answer;
    if (loop.generatedMessages.empty()) {
        Json::Value final; final["role"] = "assistant"; final["content"] = answer;
        Json::Value metadata; metadata["schema_version"] = 1; metadata["tools_used"] = loop.toolsUsed;
        metadata["tool_executions"] = publicExecutions;
        conversations_.saveReplayMessage(id, "assistant", answer, turnId, "completed", final,
                                         Json::Value(), Json::Value(), presentation, metadata);
    }
    return {answer, id, loop.toolsUsed, publicExecutions, presentation};
}
