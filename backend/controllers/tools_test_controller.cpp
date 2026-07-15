#include "tools_test_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "tools/tools.h"

void ToolsTestController::testTools(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        Json::Value response;
        response["status"] = "success";

        // Run each tool and show its output
        response["tools"]["find_delayed_flights"] = Tools::find_delayed_flights();
        response["tools"]["get_active_incidents"] = Tools::get_active_incidents();

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
