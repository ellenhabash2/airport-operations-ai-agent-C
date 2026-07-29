#include "conversation_service.h"
#include "agent/PresentationService.h"
#include "domain_error.h"
#include "repositories/conversation_repository.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <vector>

ConversationService::ConversationService() : ConversationService({
    [](const std::string &userId) { return ConversationRepository::createConversation(userId); },
    ConversationRepository::saveMessage,
    ConversationRepository::conversationBelongsToUser, ConversationRepository::getUserConversations,
    ConversationRepository::getConversationMessages,
    [](const std::string &userId, const std::string &title) { return ConversationRepository::createConversation(userId, title); },
    ConversationRepository::saveStructuredMessage,
    ConversationRepository::deleteConversationForUser}) {}
ConversationService::ConversationService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
std::string ConversationService::create(const std::string &userId) const {
    auto value = dependencies_.create(userId);
    if (value.isMember("error")) throw std::runtime_error("Conversation could not be created");
    return value["id"].asString();
}
std::string ConversationService::create(const std::string &userId, const std::string &firstMessage) const {
    if (!dependencies_.createNamed) return create(userId);
    auto value = dependencies_.createNamed(userId, titleFromFirstMessage(firstMessage));
    if (value.isMember("error")) throw std::runtime_error("Conversation could not be created");
    return value["id"].asString();
}
Json::Value ConversationService::list(const std::string &userId) const { return dependencies_.list(userId); }
void ConversationService::requireOwnership(const std::string &conversationId, const std::string &userId) const {
    if (!dependencies_.owns(conversationId, userId))
        throw DomainError(DomainErrorKind::NotFound, "conversation_not_found", "Conversation not found");
}
Json::Value ConversationService::loadOwnedMessages(const std::string &conversationId, const std::string &userId) const {
    requireOwnership(conversationId, userId);
    const auto rows = dependencies_.messages(conversationId, userId);
    Json::Value messages(Json::arrayValue);
    for (const auto &row : rows) {
        const auto role = row.get("role", "").asString();
        if (role != "user" && role != "assistant") continue;
        // Assistant tool-call rows are provider replay internals, not visible
        // chat messages. Their safe summary belongs on the final assistant row.
        if (role == "assistant" && row["tool_calls"].isArray() && !row["tool_calls"].empty())
            continue;
        Json::Value message;
        for (const char *field : {"id", "conversation_id", "role", "content", "created_at", "turn_status"})
            if (row.isMember(field)) message[field] = row[field];
        if (role == "assistant") {
            const auto &storedPresentation = row["presentation"];
            message["presentation"] = PresentationService::validate(storedPresentation)
                ? storedPresentation : Json::Value(Json::nullValue);
            const auto &metadata = row["metadata"];
            message["tool_executions"] = metadata.isObject() && metadata["tool_executions"].isArray()
                ? metadata["tool_executions"] : Json::Value(Json::arrayValue);
            message["tools_used"] = metadata.isObject() && metadata["tools_used"].isArray()
                ? metadata["tools_used"] : Json::Value(Json::arrayValue);
        }
        messages.append(message);
    }
    return messages;
}

void ConversationService::saveReplayMessage(const std::string &id, const std::string &role,
    const std::string &content, const std::string &turnId, const std::string &turnStatus,
    const Json::Value &payload, const Json::Value &calls, const Json::Value &results,
    const Json::Value &presentation, const Json::Value &metadata) const {
    if (!dependencies_.saveStructured) {
        auto value = dependencies_.save(id, role, content.empty() ? " " : content);
        if (value.isMember("error")) throw std::runtime_error("Structured message could not be saved");
        return;
    }
    auto value = dependencies_.saveStructured(id, role, content, turnId, turnStatus,
                                               payload, calls, results, presentation, metadata);
    if (value.isMember("error")) throw std::runtime_error("Structured message could not be saved");
}

Json::Value ConversationService::loadReplayHistory(const std::string &id, const std::string &userId,
                                                   std::size_t maxTurns) const {
    requireOwnership(id, userId);
    const auto rows = dependencies_.messages(id, userId);
    std::vector<Json::Value> groups; std::string currentKey;
    for (const auto &row : rows) {
        std::string key = row.get("turn_id", "").asString();
        if (key.empty()) {
            if (row.get("role", "").asString() == "user" || groups.empty())
                key = "legacy-" + std::to_string(groups.size() + 1);
            else key = currentKey;
        }
        if (groups.empty() || key != currentKey) {
            Json::Value group(Json::arrayValue); groups.push_back(group); currentKey = key;
        }
        groups.back().append(row);
    }
    std::vector<Json::Value> completeGroups;
    for (const auto &group : groups) {
        bool legacy = true, complete = false;
        for (const auto &row : group) {
            if (row.isMember("turn_id")) legacy = false;
            if (row.get("turn_status", "").asString() == "completed") complete = true;
        }
        if (legacy || complete) completeGroups.push_back(group);
    }
    if (maxTurns < 1) maxTurns = 1;
    if (maxTurns > 100) maxTurns = 100;
    const auto first = completeGroups.size() > maxTurns ? completeGroups.size() - maxTurns : 0;
    Json::Value replay(Json::arrayValue);
    for (std::size_t index = first; index < completeGroups.size(); ++index) {
        for (const auto &row : completeGroups[index]) {
            Json::Value message;
            const auto role = row.get("role", "").asString();
            if (role != "user" && role != "assistant" && role != "system" && role != "tool") continue;
            const auto &payload = row["provider_payload"];
            if (payload.isObject() && payload.get("role", "").asString() == role) message = payload;
            else { message["role"] = role; message["content"] = row.get("content", "");
                   if (row["tool_calls"].isArray()) message["tool_calls"] = row["tool_calls"]; }
            if (role == "tool" && !message.isMember("tool_call_id")) {
                const auto &meta = row["metadata"];
                if (meta.isObject() && meta["tool_call_id"].isString()) message["tool_call_id"] = meta["tool_call_id"];
                else continue;
            }
            replay.append(message);
        }
    }
    return replay;
}

void ConversationService::deleteConversation(const std::string &id, const std::string &userId) const {
    if (id.empty() || !std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isdigit(c); }) || id == "0")
        throw DomainError(DomainErrorKind::Validation, "invalid_conversation_id", "Conversation ID must be a positive integer");
    if (!dependencies_.remove || !dependencies_.remove(id, userId))
        throw DomainError(DomainErrorKind::NotFound, "conversation_not_found", "Conversation not found");
}

std::string ConversationService::titleFromFirstMessage(const std::string &input) {
    std::string normalized; bool space = false;
    for (unsigned char c : input) {
        if (std::isspace(c)) { space = !normalized.empty(); continue; }
        if (space) { normalized.push_back(' '); space = false; }
        normalized.push_back(static_cast<char>(c));
    }
    if (normalized.empty()) return "New conversation";
    constexpr std::size_t maximum = 80;
    if (normalized.size() > maximum) normalized = normalized.substr(0, maximum - 3) + "...";
    return normalized;
}

std::size_t ConversationService::historyMaxTurnsFromEnvironment() {
    const char *raw = std::getenv("AGENT_HISTORY_MAX_TURNS");
    if (!raw || !*raw) return 30;
    try { const long value = std::stol(raw); return static_cast<std::size_t>(std::clamp(value, 1L, 100L)); }
    catch (...) { return 30; }
}
Json::Value ConversationService::saveUserMessage(const std::string &id, const std::string &content) const {
    auto value = dependencies_.save(id, "user", content);
    if (value.isMember("error")) throw std::runtime_error("User message could not be saved");
    return value;
}
Json::Value ConversationService::saveAssistantMessage(const std::string &id, const std::string &content) const {
    auto value = dependencies_.save(id, "assistant", content);
    if (value.isMember("error")) throw std::runtime_error("Assistant response could not be saved");
    return value;
}
