#include "gate_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "repositories/gate_repository.h"

void GateController::getGates(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto gates = GateRepository::getAllGates();
        
        Json::Value response;
        response["status"] = "success";
        response["data"] = gates;
        
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
