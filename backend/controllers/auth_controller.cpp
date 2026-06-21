#include "auth_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>

void AuthController::registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value response;
    response["status"] = "planned";
    response["message"] = "User registration is reserved for the authentication phase";
    response["phase"] = "Foundation Phase";
    
    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(k501NotImplemented);
    callback(http_response);
}

void AuthController::loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value response;
    response["status"] = "planned";
    response["message"] = "User login is reserved for the authentication phase";
    response["phase"] = "Foundation Phase";
    
    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(k501NotImplemented);
    callback(http_response);
}
