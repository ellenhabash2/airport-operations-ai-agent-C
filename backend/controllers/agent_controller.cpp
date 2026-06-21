#include "agent_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>

void AgentController::queryAgent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value response;
    response["status"] = "planned";
    response["message"] = "AI Agent querying is reserved for the Gemini and agentic loop phase";
    response["phase"] = "Foundation Phase";
    response["todo"] = Json::arrayValue;
    response["todo"][0] = "Implement Gemini API integration";
    response["todo"][1] = "Define AI function tools";
    response["todo"][2] = "Build Agentic Loop";
    
    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(k501NotImplemented);
    callback(http_response);
}

void AgentController::getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value response;
    response["status"] = "planned";
    response["message"] = "Chat history retrieval is reserved for the conversation memory phase";
    response["phase"] = "Foundation Phase";
    
    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(k501NotImplemented);
    callback(http_response);
}
