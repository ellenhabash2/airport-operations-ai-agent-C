#include "agent_controller.h"
#include <drogon/HttpAppFramework.h>
#include <iostream>
#include <json/json.h>
#include "agent/LLMClient.h"
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

        // "query" present but blank (empty or only whitespace) is not a usable
        // question; reject it up front instead of sending an empty turn to the model.
        {
            const auto first = userQuery.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
            {
                Json::Value error_response;
                error_response["error"] = "Field 'query' must not be empty";
                auto http_response = HttpResponse::newHttpJsonResponse(error_response);
                http_response->setStatusCode(k400BadRequest);
                callback(http_response);
                return;
            }
        }

        LLMClient client;
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
                    // but some return it as an already-parsed object. Handle both.
                    Json::Value args(Json::objectValue);
                    bool argsValid = true;
                    std::string parseErr;

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
                            std::unique_ptr<Json::CharReader> jr(rb.newCharReader());
                            argsValid = jr->parse(argsStr.c_str(), argsStr.c_str() + argsStr.size(), &args, &parseErr)
                                        && args.isObject();
                        }
                    }

                    toolsUsed.append(toolName);

                    // Build the tool result. Malformed arguments or an exception
                    // thrown by a tool must not crash the whole request: report the
                    // problem back to the model as the tool's result so it can react.
                    Json::Value toolResult;
                    if (!argsValid)
                    {
                        toolResult["error"] = "Invalid tool arguments" +
                                              (parseErr.empty() ? std::string() : ": " + parseErr);
                    }
                    else
                    {
                        try
                        {
                            toolResult = ToolRegistry::executeTool(toolName, args);
                        }
                        catch (const std::exception &toolEx)
                        {
                            toolResult = Json::Value(Json::objectValue);
                            toolResult["error"] = std::string("Tool execution failed: ") + toolEx.what();
                        }
                    }

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

        // If the model kept calling tools and never wrote a final answer within
        // the step budget, ask it once more with NO tools so it must summarize
        // what it already gathered into a useful reply.
        if (finalAnswer.empty())
        {
            Json::Value summaryResp = client.chatWithTools(messages, Json::Value(Json::arrayValue));
            if (!summaryResp.isMember("error") &&
                summaryResp.isMember("choices") && summaryResp["choices"].isArray() &&
                !summaryResp["choices"].empty())
            {
                const Json::Value &m = summaryResp["choices"][0]["message"];
                if (m.isMember("content") && !m["content"].isNull())
                {
                    finalAnswer = m["content"].asString();
                }
            }
        }

        // Absolute fallback so the client always has something to show.
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
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";
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
