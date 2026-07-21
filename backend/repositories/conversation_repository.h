#pragma once

#include <json/json.h>
#include <string>

// Handles persistence and retrieval of AI-agent conversations and messages.
class ConversationRepository
{
public:
    // Creates a new conversation for the authenticated user.
    //
    // Returns:
    // {
    //     "id": "...",
    //     "user_id": "...",
    //     "title": "...",
    //     "created_at": "..."
    // }
    //
    // Returns an object containing "error" when validation fails.
    static Json::Value createConversation(
        const std::string &userId,
        const std::string &title = "Airport Operations Conversation");

    // Saves one message in an existing conversation.
    //
    // Supported roles:
    // user, assistant, system, tool
    //
    // Returns the created message, or an object containing "error".
    static Json::Value saveMessage(
        const std::string &conversationId,
        const std::string &role,
        const std::string &content);

    // Returns true only when the conversation belongs to the given user.
    static bool conversationBelongsToUser(
        const std::string &conversationId,
        const std::string &userId);

    // Returns all conversations belonging to the given user,
    // newest conversation first.
    static Json::Value getUserConversations(
        const std::string &userId);

    // Returns all messages from one conversation in chronological order.
    //
    // The user id is included in the query so messages belonging to another
    // user are never returned accidentally.
    static Json::Value getConversationMessages(
        const std::string &conversationId,
        const std::string &userId);
};