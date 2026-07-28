#include "weather_controller.h"
#include <iostream>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "services/domain_error.h"
#include "services/weather_service.h"

void WeatherController::getWeather(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto weather = WeatherService{}.getRecent();
        
        Json::Value response;
        response["status"] = "success";
        response["data"] = weather;
        
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

void WeatherController::createWeather(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto json = req->getJsonObject();
        
        if (!json || !json->isMember("condition") || !json->isMember("visibility_km") || !json->isMember("wind_speed_kmh") || !json->isMember("temperature_c"))
        {
            Json::Value error_response;
            error_response["error"] = "Missing required fields: condition, visibility_km, wind_speed_kmh, temperature_c";
            
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        std::string condition = (*json)["condition"].asString();
        float visibility_km = (*json)["visibility_km"].asFloat();
        float wind_speed_kmh = (*json)["wind_speed_kmh"].asFloat();
        float temperature_c = (*json)["temperature_c"].asFloat();

        auto weather = WeatherService{}.create(condition, visibility_km, wind_speed_kmh, temperature_c);
        
        Json::Value response;
        response["status"] = "success";
        response["data"] = weather;
        
        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k201Created);
        callback(http_response);
    }
    catch (const DomainError &e)
    {
        Json::Value error_response; error_response["error"] = e.what();
        auto response = HttpResponse::newHttpJsonResponse(error_response);
        response->setStatusCode(k400BadRequest); callback(response);
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
