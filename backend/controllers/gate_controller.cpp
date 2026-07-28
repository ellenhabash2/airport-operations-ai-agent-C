#include "gate_controller.h"
#include <iostream>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "services/gate_service.h"

void GateController::getGates(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto gates = GateService{}.getAll();
        
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
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";
        
        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}
