#include "flight_controller.h"
#include <iostream>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "services/domain_error.h"
#include "services/flight_service.h"

void FlightController::getFlights(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto flights = FlightService{}.getAll();

        Json::Value response;
        response["status"] = "success";
        response["data"] = flights;

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

void FlightController::getDelayedFlights(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto flights = FlightService{}.getDelayed();

        Json::Value response;
        response["status"] = "success";
        response["count"] = flights.size();
        response["data"] = flights;

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

void FlightController::getFlightById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try
    {
        auto flight = FlightService{}.getById(id);
        Json::Value response;
        response["status"] = "success";
        response["data"] = flight;
        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k200OK);
        callback(http_response);
    }
    catch (const DomainError &e)
    {
        Json::Value error; error["error"] = e.what();
        auto response = HttpResponse::newHttpJsonResponse(error);
        response->setStatusCode(e.kind() == DomainErrorKind::Validation ? k400BadRequest : k404NotFound);
        callback(response);
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
