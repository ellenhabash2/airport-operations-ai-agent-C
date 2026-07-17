#include "agent_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "agent/GeminiClient.h"
#include "agent/ToolRegistry.h"
#include <memory>

void AgentController::queryAgent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto json = req->getJsonObject();
        if (!json || !json->isMember("query"))
        {
            Json::Value error_response;
            error_response["error"] = "Missing required field: query";
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        std::string userQuery = (*json)["query"].asString();

        GeminiClient client;
        Json::Value tools = ToolRegistry::getToolDefinitions();

        // Conversation starts with a system prompt + the user question.
        Json::Value messages(Json::arrayValue);

        Json::Value systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] =
            "You are AeroMind, an AI assistant for airport operations. "
            "Use the provided tools to answer questions about flights, gates, runways, "
            "incidents, and weather. You may call multiple tools in sequence if needed. "
            "When you have enough information, give a clear, concise final answer.";
        messages.append(systemMsg);

        Json::Value userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = userQuery;
        messages.append(userMsg);

        // Track which tools were called (for transparency / grading demo).
        Json::Value toolsUsed(Json::arrayValue);
        std::string finalAnswer;

        // Agentic loop: up to 5 iterations of tool calls.
        for (int step = 0; step < 5; ++step)
        {
            Json::Value response = client.chatWithTools(messages, tools);

            if (response.isMember("error"))
            {
                Json::Value error_response;
                error_response["error"] = "AI provider error: " + response["error"].asString();
                auto http_response = HttpResponse::newHttpJsonResponse(error_response);
                http_response->setStatusCode(k502BadGateway);
                callback(http_response);
                return;
            }

            if (!response.isMember("choices") || !response["choices"].isArray() || response["choices"].empty())
            {
                break;
            }

            const Json::Value &choice = response["choices"][0];
            const Json::Value &message = choice["message"];

            // Case 1: the model wants to call one or more tools.
            if (message.isMember("tool_calls") && message["tool_calls"].isArray() && !message["tool_calls"].empty())
            {
                // Append the assistant message (with tool_calls) to the conversation.
                messages.append(message);

                for (const auto &toolCall : message["tool_calls"])
                {
                    std::string toolName = toolCall["function"]["name"].asString();
                    const Json::Value &rawArgs = toolCall["function"]["arguments"];

                    // OpenAI-style providers send "arguments" as a JSON *string*,
                    // but some return it as an already-parsed object. Handle both,
                    // and never let a malformed value throw out of the loop.
                    Json::Value args(Json::objectValue);
                    if (rawArgs.isObject())
                    {
                        args = rawArgs;
                    }
                    else if (rawArgs.isString())
                    {
                        std::string argsStr = rawArgs.asString();
                        if (!argsStr.empty())
                        {
                            Json::CharReaderBuilder rb;
                            std::string parseErr;
                            std::unique_ptr<Json::CharReader> jr(rb.newCharReader());
                            // If parsing fails, args stays an empty object and the
                            // tool runs with its defaults rather than crashing.
                            jr->parse(argsStr.c_str(), argsStr.c_str() + argsStr.size(), &args, &parseErr);
                            if (!args.isObject())
                            {
                                args = Json::Value(Json::objectValue);
                            }
                        }
                    }

                    // Execute the tool.
                    Json::Value toolResult = ToolRegistry::executeTool(toolName, args);
                    toolsUsed.append(toolName);

                    // Append the tool result back into the conversation.
                    Json::StreamWriterBuilder w;
                    w["indentation"] = "";
                    std::string resultStr = Json::writeString(w, toolResult);

                    Json::Value toolMsg;
                    toolMsg["role"] = "tool";
                    toolMsg["tool_call_id"] = toolCall["id"];
                    toolMsg["content"] = resultStr;
                    messages.append(toolMsg);
                }
                // Loop again so the model can use the tool results.
                continue;
            }

            // Case 2: the model gave a final text answer.
            if (message.isMember("content") && !message["content"].isNull())
            {
                finalAnswer = message["content"].asString();
            }
            break;
        }

        // If the model kept calling tools and never produced a final text
        // answer within the step budget, return a clear message instead of an
        // empty string so the client always has something to show.
        if (finalAnswer.empty())
        {
            finalAnswer = "I couldn't reach a final answer within the allowed "
                          "number of steps. Please try rephrasing your question.";
        }

        Json::Value response;
        response["status"] = "success";
        response["query"] = userQuery;
        response["answer"] = finalAnswer;
        response["tools_used"] = toolsUsed;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k200OK);
        callback(http_response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        error_response["error"] = e.what();
        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}

void AgentController::getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value response;
    response["status"] = "planned";
    response["message"] = "Chat history retrieval will be added with conversation memory";
    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(k200OK);
    callback(http_response);
}
