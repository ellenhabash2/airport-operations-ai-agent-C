#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class AgentController : public HttpController<AgentController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AgentController::queryAgent, "/agent/query", Post, "JwtAuthFilter");
    ADD_METHOD_TO(AgentController::getHistory, "/agent/history", Get,"JwtAuthFilter");
    ADD_METHOD_TO(
    AgentController::getConversationMessages,
    "/agent/conversations/{1}/messages",
    Get,
    "JwtAuthFilter");
    ADD_METHOD_TO(AgentController::deleteConversation, "/agent/conversations/{1}", Delete, "JwtAuthFilter");
    METHOD_LIST_END

    void queryAgent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getConversationMessages(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &conversationId);
    void deleteConversation(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback,
                            const std::string &conversationId);
};
