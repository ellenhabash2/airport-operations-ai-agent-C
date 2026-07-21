#include "agent_controller.h"
#include <drogon/HttpAppFramework.h>
#include <iostream>
#include <json/json.h>
#include "agent/LLMClient.h"
#include "agent/ToolRegistry.h"
#include <memory>
#include "repositories/conversation_repository.h"
#include "security/JwtService.h"

namespace
{

HttpResponsePtr createJsonErrorResponse(
    const std::string &message,
    HttpStatusCode statusCode)
{
    Json::Value errorResponse;
    errorResponse["error"] = message;

    auto response = HttpResponse::newHttpJsonResponse(errorResponse);
    response->setStatusCode(statusCode);

    return response;
}

std::string extractBearerToken(const HttpRequestPtr &req)
{
    const std::string authorizationHeader =
        req->getHeader("Authorization");

    const std::string prefix = "Bearer ";

    if (authorizationHeader.rfind(prefix, 0) != 0)
    {
        return "";
    }

    return authorizationHeader.substr(prefix.size());
}

// Supports both:
// "conversation_id": 12
// and:
// "conversation_id": "12"
std::string readConversationId(const Json::Value &json)
{
    if (!json.isMember("conversation_id") ||
        json["conversation_id"].isNull())
    {
        return "";
    }

    const Json::Value &value = json["conversation_id"];

    if (value.isInt() || value.isUInt() ||
        value.isInt64() || value.isUInt64())
    {
        return value.asString();
    }

    if (value.isString())
    {
        return value.asString();
    }

    return "";
}

} // namespace


void AgentController::queryAgent(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        /*
         * The JwtAuthFilter already verified the token before this
         * controller was called. We extract the authenticated user id
         * from the same token so conversations can be associated with
         * their owner.
         */
        const std::string token = extractBearerToken(req);
        const std::string userId = JwtService::getUserId(token);

        if (userId.empty())
        {
            callback(createJsonErrorResponse(
                "Invalid or expired token",
                k401Unauthorized));
            return;
        }

        auto json = req->getJsonObject();

        if (!json || !json->isMember("query") ||
            !(*json)["query"].isString())
        {
            callback(createJsonErrorResponse(
                "Missing required field: query",
                k400BadRequest));
            return;
        }

        std::string userQuery = (*json)["query"].asString();

        if (userQuery.find_first_not_of(" \t\r\n") ==
            std::string::npos)
        {
            callback(createJsonErrorResponse(
                "Field 'query' must not be empty",
                k400BadRequest));
            return;
        }

        /*
         * If conversation_id was supplied, continue that conversation.
         * Otherwise, create a new conversation owned by this user.
         */
        std::string conversationId;

        if (json->isMember("conversation_id") &&
            !(*json)["conversation_id"].isNull())
        {
            conversationId = readConversationId(*json);

            if (conversationId.empty())
            {
                callback(createJsonErrorResponse(
                    "Field 'conversation_id' must be a valid integer",
                    k400BadRequest));
                return;
            }

            if (!ConversationRepository::conversationBelongsToUser(
                    conversationId,
                    userId))
            {
                callback(createJsonErrorResponse(
                    "You do not have access to this conversation",
                    k403Forbidden));
                return;
            }
        }
        else
        {
            Json::Value conversation =
                ConversationRepository::createConversation(userId);

            if (conversation.isMember("error"))
            {
                callback(createJsonErrorResponse(
                    "Conversation could not be created",
                    k500InternalServerError));
                return;
            }

            conversationId = conversation["id"].asString();
        }

        LLMClient client;
        Json::Value tools = ToolRegistry::getToolDefinitions();

        Json::Value messages(Json::arrayValue);

        /*
         * The system prompt is application configuration rather than
         * user conversation history, so it is added on every AI request
         * but does not need to be stored in the database.
         */
        Json::Value systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] =
            "You are AeroMind, an AI assistant for airport operations. "
            "Use the provided tools to answer questions about flights, "
            "gates, runways, incidents, and weather. "
            "You may call multiple tools in sequence if needed. "
            "When you have enough information, give a clear, concise "
            "final answer.";

        messages.append(systemMsg);

        /*
         * Load previous user and assistant messages before adding the
         * new user message. The repository query also checks ownership.
         */
        Json::Value previousMessages =
            ConversationRepository::getConversationMessages(
                conversationId,
                userId);

        for (const auto &storedMessage : previousMessages)
        {
            if (!storedMessage.isMember("role") ||
                !storedMessage.isMember("content"))
            {
                continue;
            }

            const std::string role =
                storedMessage["role"].asString();

            /*
             * Persisted user and assistant messages can safely be sent
             * back to the provider as conversation context.
             *
             * Old tool messages are intentionally not replayed because
             * an OpenAI-compatible tool message requires its matching
             * assistant tool_call and tool_call_id from the same turn.
             */
            if (role != "user" && role != "assistant")
            {
                continue;
            }

            Json::Value historyMessage;
            historyMessage["role"] = role;
            historyMessage["content"] =
                storedMessage["content"].asString();

            messages.append(historyMessage);
        }

        /*
         * Save the new user turn before starting the agent loop.
         */
        Json::Value savedUserMessage =
            ConversationRepository::saveMessage(
                conversationId,
                "user",
                userQuery);

        if (savedUserMessage.isMember("error"))
        {
            callback(createJsonErrorResponse(
                "User message could not be saved",
                k500InternalServerError));
            return;
        }

        Json::Value userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = userQuery;
        messages.append(userMsg);

        Json::Value toolsUsed(Json::arrayValue);
        std::string finalAnswer;

        // Agentic loop: up to five tool-calling iterations.
        for (int step = 0; step < 5; ++step)
        {
            Json::Value aiResponse =
                client.chatWithTools(messages, tools);

            if (aiResponse.isMember("error"))
            {
                std::cerr
                    << "AI provider error: "
                    << aiResponse["error"].asString()
                    << std::endl;


                callback(createJsonErrorResponse(
                    "AI provider is currently unavailable",
                    k502BadGateway));
                return;
            }

            if (!aiResponse.isMember("choices") ||
                !aiResponse["choices"].isArray() ||
                aiResponse["choices"].empty())
            {
                break;
            }

            const Json::Value &choice =
                aiResponse["choices"][0];

            if (!choice.isMember("message") ||
                !choice["message"].isObject())
            {
                break;
            }

            const Json::Value &message =
                choice["message"];

            /*
             * The model requested one or more function tools.
             */
            if (message.isMember("tool_calls") &&
                message["tool_calls"].isArray() &&
                !message["tool_calls"].empty())
            {
                messages.append(message);

                for (const auto &toolCall :
                     message["tool_calls"])
                {
                    const std::string toolName =
                        toolCall["function"]["name"].asString();

                    const Json::Value &rawArgs =
                        toolCall["function"]["arguments"];

                    Json::Value args(Json::objectValue);
                    bool argsValid = true;
                    std::string parseError;

                    if (rawArgs.isObject())
                    {
                        args = rawArgs;
                    }
                    else if (rawArgs.isString())
                    {
                        const std::string argsString =
                            rawArgs.asString();

                        if (!argsString.empty())
                        {
                            Json::CharReaderBuilder readerBuilder;

                            std::unique_ptr<Json::CharReader> reader(
                                readerBuilder.newCharReader());

                            argsValid =
                                reader->parse(
                                    argsString.c_str(),
                                    argsString.c_str() +
                                        argsString.size(),
                                    &args,
                                    &parseError) &&
                                args.isObject();
                        }
                    }
                    else
                    {
                        argsValid = false;
                    }

                    toolsUsed.append(toolName);

                    Json::Value toolResult;

                    if (!argsValid)
                    {
                        toolResult["error"] =
                            "Invalid tool arguments";
                    }
                    else
                    {
                        try
                        {
                            toolResult =
                                ToolRegistry::executeTool(
                                    toolName,
                                    args);
                        }
                        catch (const std::exception &toolException)
                        {
                            /*
                             * Log internal details server-side, but do
                             * not return them to the AI provider.
                             */
                            std::cerr
                                << "Tool execution error in "
                                << toolName
                                << ": "
                                << toolException.what()
                                << std::endl;

                            toolResult =
                                Json::Value(Json::objectValue);

                            toolResult["error"] =
                                "Tool execution failed";
                        }
                    }

                    Json::StreamWriterBuilder writer;
                    writer["indentation"] = "";

                    const std::string resultString =
                        Json::writeString(
                            writer,
                            toolResult);

                    Json::Value toolMessage;
                    toolMessage["role"] = "tool";
                    toolMessage["tool_call_id"] =
                        toolCall["id"];
                    toolMessage["content"] =
                        resultString;

                    messages.append(toolMessage);
                }

                continue;
            }

            /*
             * The model returned its final natural-language answer.
             */
            if (message.isMember("content") &&
                !message["content"].isNull())
            {
                finalAnswer =
                    message["content"].asString();
            }

            break;
        }

        /*
         * If the model used the full tool-call budget, ask it once more
         * without tools so it must summarize the collected results.
         */
        if (finalAnswer.empty())
        {
            Json::Value noTools(Json::arrayValue);

            Json::Value summaryResponse =
                client.chatWithTools(messages, noTools);

            if (!summaryResponse.isMember("error") &&
                summaryResponse.isMember("choices") &&
                summaryResponse["choices"].isArray() &&
                !summaryResponse["choices"].empty())
            {
                const Json::Value &summaryMessage =
                    summaryResponse["choices"][0]["message"];

                if (summaryMessage.isMember("content") &&
                    !summaryMessage["content"].isNull())
                {
                    finalAnswer =
                        summaryMessage["content"].asString();
                }
            }
        }

        if (finalAnswer.empty())
        {
            finalAnswer =
                "I couldn't reach a final answer within the allowed "
                "number of steps. Please try rephrasing your question.";
        }

        /*
         * Persist only the final answer that is visible to the user.
         * Temporary assistant tool-call objects and tool outputs are
         * used inside the current agent loop, but are not required for
         * long-term conversational memory.
         */
        Json::Value savedAssistantMessage =
            ConversationRepository::saveMessage(
                conversationId,
                "assistant",
                finalAnswer);

        if (savedAssistantMessage.isMember("error"))
        {
            callback(createJsonErrorResponse(
                "Assistant response could not be saved",
                k500InternalServerError));
            return;
        }

        Json::Value response;
        response["status"] = "success";
        response["conversation_id"] = conversationId;
        response["query"] = userQuery;
        response["answer"] = finalAnswer;
        response["tools_used"] = toolsUsed;

        auto httpResponse =
            HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(k200OK);
        callback(httpResponse);
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Agent request error: "
            << e.what()
            << std::endl;

        callback(createJsonErrorResponse(
            "Internal server error",
            k500InternalServerError));
    }
}



void AgentController::getHistory(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        // JwtAuthFilter already validated the token.
        const std::string token = extractBearerToken(req);
        const std::string userId = JwtService::getUserId(token);

        if (userId.empty())
        {
            callback(createJsonErrorResponse(
                "Invalid or expired token",
                k401Unauthorized));
            return;
        }

        Json::Value response;
        response["status"] = "success";
        response["conversations"] =
            ConversationRepository::getUserConversations(userId);

        auto httpResponse =
            HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(k200OK);
        callback(httpResponse);
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "History request error: "
            << e.what()
            << std::endl;

        callback(createJsonErrorResponse(
            "Internal server error",
            k500InternalServerError));
    }
}

void AgentController::getConversationMessages(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &conversationId)
{
    try
    {
        // JwtAuthFilter already validated the token.
        const std::string token = extractBearerToken(req);
        const std::string userId = JwtService::getUserId(token);

        if (userId.empty())
        {
            callback(createJsonErrorResponse(
                "Invalid or expired token",
                k401Unauthorized));
            return;
        }

        Json::Value response;
        response["status"] = "success";
        response["conversation_id"] = conversationId;
        response["messages"] =
            ConversationRepository::getConversationMessages(
                conversationId,
                userId);

        auto httpResponse =
            HttpResponse::newHttpJsonResponse(response);

        httpResponse->setStatusCode(k200OK);
        callback(httpResponse);
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "Conversation messages request error: "
            << e.what()
            << std::endl;

        callback(createJsonErrorResponse(
            "Internal server error",
            k500InternalServerError));
    }
}