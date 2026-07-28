#include "conversation_service.h"
#include "domain_error.h"
#include "repositories/conversation_repository.h"

ConversationService::ConversationService() : ConversationService({
    [](const std::string &userId) { return ConversationRepository::createConversation(userId); },
    ConversationRepository::saveMessage,
    ConversationRepository::conversationBelongsToUser, ConversationRepository::getUserConversations,
    ConversationRepository::getConversationMessages}) {}
ConversationService::ConversationService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
std::string ConversationService::create(const std::string &userId) const {
    auto value = dependencies_.create(userId);
    if (value.isMember("error")) throw std::runtime_error("Conversation could not be created");
    return value["id"].asString();
}
Json::Value ConversationService::list(const std::string &userId) const { return dependencies_.list(userId); }
void ConversationService::requireOwnership(const std::string &conversationId, const std::string &userId) const {
    if (!dependencies_.owns(conversationId, userId))
        throw DomainError(DomainErrorKind::Forbidden, "conversation_forbidden", "You do not have access to this conversation");
}
Json::Value ConversationService::loadOwnedMessages(const std::string &conversationId, const std::string &userId) const {
    requireOwnership(conversationId, userId);
    return dependencies_.messages(conversationId, userId);
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
