#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class AgentController : public HttpController<AgentController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AgentController::queryAgent, "/agent/query", Post);
    ADD_METHOD_TO(AgentController::getHistory, "/agent/history", Get);
    METHOD_LIST_END

    void queryAgent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
