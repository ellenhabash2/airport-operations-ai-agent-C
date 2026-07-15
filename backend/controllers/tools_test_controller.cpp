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

        // Run the read tools and show their output.
        // Action tools (resolve_incident, create_incident) are not run here
        // because they modify data.
        response["tools"]["find_delayed_flights"] = Tools::find_delayed_flights();
        response["tools"]["get_active_incidents"] = Tools::get_active_incidents();
        response["tools"]["get_all_flights_count"] = Tools::get_all_flights().size();
        response["tools"]["get_flight_details_id_1"] = Tools::get_flight_details("1");
        response["tools"]["get_available_gates"] = Tools::get_available_gates();
        response["tools"]["get_runway_status"] = Tools::get_runway_status();
        response["tools"]["get_latest_weather"] = Tools::get_latest_weather();

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
