#include "health_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>

void HealthController::health(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    Json::Value response;
    response["status"] = "ok";
    response["service"] = "AeroMind";

    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(k200OK);
    callback(http_response);
}
