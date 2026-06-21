#include "runway_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "repositories/runway_repository.h"

void RunwayController::getRunways(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto runways = RunwayRepository::getAllRunways();
        
        Json::Value response;
        response["status"] = "success";
        response["data"] = runways;
        
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
