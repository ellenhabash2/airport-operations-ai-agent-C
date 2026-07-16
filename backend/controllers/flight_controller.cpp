#include "flight_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "database/database_manager.h"
#include "repositories/flight_repository.h"
#include <algorithm>
#include <cctype>

void FlightController::getFlights(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto flights = FlightRepository::getAllFlights();

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
        error_response["error"] = e.what();

        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}

void FlightController::getDelayedFlights(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto flights = FlightRepository::getDelayedFlights();

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
        error_response["error"] = e.what();

        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}

void FlightController::getFlightById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try
    {
        if (id.empty() || !std::all_of(id.begin(), id.end(), [](unsigned char ch) { return std::isdigit(ch); }))
        {
            Json::Value error_response;
            error_response["error"] = "Flight ID must be a positive integer";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        auto result = FlightRepository::getFlightById(id);

        Json::Value response;
        if (!result["found"].asBool())
        {
            response["error"] = "Flight not found";
            auto http_response = HttpResponse::newHttpJsonResponse(response);
            http_response->setStatusCode(k404NotFound);
            callback(http_response);
        }
        else
        {
            response["status"] = "success";
            response["data"] = result["flight"];
            auto http_response = HttpResponse::newHttpJsonResponse(response);
            http_response->setStatusCode(k200OK);
            callback(http_response);
        }
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
