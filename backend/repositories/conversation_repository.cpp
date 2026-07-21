#include "conversation_repository.h"

#include "database/database_manager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <pqxx/pqxx>

namespace
{

// IDs received from JWTs, URLs, or JSON bodies are strings in the current
// project. Validate them before passing them to PostgreSQL integer columns.
bool isPositiveInteger(const std::string &value)
{
    return !value.empty() &&
           std::all_of(
               value.begin(),
               value.end(),
               [](unsigned char ch)
               {
                   return std::isdigit(ch);
               });
}

// The role must satisfy the messages_role_check constraint in init.sql.
bool isValidRole(const std::string &role)
{
    static constexpr std::array<const char *, 4> allowedRoles = {
        "user",
        "assistant",
        "system",
        "tool"
    };

    return std::find(
               allowedRoles.begin(),
               allowedRoles.end(),
               role) != allowedRoles.end();
}

// Converts one conversations row into the JSON structure used by the backend.
Json::Value conversationRowToJson(const pqxx::row &row)
{
    Json::Value conversation;

    conversation["id"] = row["id"].c_str();
    conversation["user_id"] = row["user_id"].c_str();
    conversation["title"] = row["title"].c_str();
    conversation["created_at"] = row["created_at"].c_str();

    return conversation;
}

// Converts one messages row into JSON.
Json::Value messageRowToJson(const pqxx::row &row)
{
    Json::Value message;

    message["id"] = row["id"].c_str();
    message["conversation_id"] = row["conversation_id"].c_str();
    message["role"] = row["role"].c_str();
    message["content"] = row["content"].c_str();
    message["created_at"] = row["created_at"].c_str();

    return message;
}

} // namespace

Json::Value ConversationRepository::createConversation(
    const std::string &userId,
    const std::string &title)
{
    Json::Value result;

    if (!isPositiveInteger(userId))
    {
        result["error"] = "User id must be a positive integer.";
        return result;
    }

    const std::string conversationTitle =
        title.empty()
            ? "Airport Operations Conversation"
            : title;

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result rows = txn.exec_params(
            "INSERT INTO conversations (user_id, title) "
            "VALUES ($1, $2) "
            "RETURNING id, user_id, title, created_at",
            userId,
            conversationTitle);

        txn.commit();

        if (rows.empty())
        {
            result["error"] = "Conversation could not be created.";
            return result;
        }

        return conversationRowToJson(rows[0]);
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Database error (createConversation): "
            << e.what()
            << std::endl;

        throw;
    }
}

Json::Value ConversationRepository::saveMessage(
    const std::string &conversationId,
    const std::string &role,
    const std::string &content)
{
    Json::Value result;

    if (!isPositiveInteger(conversationId))
    {
        result["error"] = "Conversation id must be a positive integer.";
        return result;
    }

    if (!isValidRole(role))
    {
        result["error"] =
            "Message role must be one of: user, assistant, system, tool.";

        return result;
    }

    if (content.empty())
    {
        result["error"] = "Message content must not be empty.";
        return result;
    }

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result rows = txn.exec_params(
            "INSERT INTO messages (conversation_id, role, content) "
            "VALUES ($1, $2, $3) "
            "RETURNING id, conversation_id, role, content, created_at",
            conversationId,
            role,
            content);

        txn.commit();

        if (rows.empty())
        {
            result["error"] = "Message could not be saved.";
            return result;
        }

        return messageRowToJson(rows[0]);
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Database error (saveMessage): "
            << e.what()
            << std::endl;

        throw;
    }
}

bool ConversationRepository::conversationBelongsToUser(
    const std::string &conversationId,
    const std::string &userId)
{
    if (!isPositiveInteger(conversationId) ||
        !isPositiveInteger(userId))
    {
        return false;
    }

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result rows = txn.exec_params(
            "SELECT EXISTS ("
            "    SELECT 1 "
            "    FROM conversations "
            "    WHERE id = $1 AND user_id = $2"
            ") AS belongs_to_user",
            conversationId,
            userId);

        txn.commit();

        return !rows.empty() &&
               rows[0]["belongs_to_user"].as<bool>();
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Database error (conversationBelongsToUser): "
            << e.what()
            << std::endl;

        throw;
    }
}

Json::Value ConversationRepository::getUserConversations(
    const std::string &userId)
{
    Json::Value conversations(Json::arrayValue);

    if (!isPositiveInteger(userId))
    {
        return conversations;
    }

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result rows = txn.exec_params(
            "SELECT "
            "    c.id, "
            "    c.user_id, "
            "    c.title, "
            "    c.created_at, "
            "    COUNT(m.id) AS message_count, "
            "    MAX(m.created_at) AS last_message_at "
            "FROM conversations c "
            "LEFT JOIN messages m "
            "    ON m.conversation_id = c.id "
            "WHERE c.user_id = $1 "
            "GROUP BY c.id, c.user_id, c.title, c.created_at "
            "ORDER BY COALESCE(MAX(m.created_at), c.created_at) DESC, "
            "         c.id DESC",
            userId);

        for (const auto &row : rows)
        {
            Json::Value conversation =
                conversationRowToJson(row);

            conversation["message_count"] =
                row["message_count"].as<int>();

            conversation["last_message_at"] =
                row["last_message_at"].is_null()
                    ? ""
                    : row["last_message_at"].c_str();

            conversations.append(conversation);
        }

        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Database error (getUserConversations): "
            << e.what()
            << std::endl;

        throw;
    }

    return conversations;
}

Json::Value ConversationRepository::getConversationMessages(
    const std::string &conversationId,
    const std::string &userId)
{
    Json::Value messages(Json::arrayValue);

    if (!isPositiveInteger(conversationId) ||
        !isPositiveInteger(userId))
    {
        return messages;
    }

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        // Joining conversations here provides an additional ownership check:
        // even if this method is called without a prior authorization check,
        // it cannot return another user's messages.
        pqxx::result rows = txn.exec_params(
            "SELECT "
            "    m.id, "
            "    m.conversation_id, "
            "    m.role, "
            "    m.content, "
            "    m.created_at "
            "FROM messages m "
            "JOIN conversations c "
            "    ON c.id = m.conversation_id "
            "WHERE m.conversation_id = $1 "
            "  AND c.user_id = $2 "
            "ORDER BY m.created_at ASC, m.id ASC",
            conversationId,
            userId);

        for (const auto &row : rows)
        {
            messages.append(messageRowToJson(row));
        }

        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Database error (getConversationMessages): "
            << e.what()
            << std::endl;

        throw;
    }

    return messages;
}