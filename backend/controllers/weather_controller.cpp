#include "weather_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "repositories/weather_repository.h"

void WeatherController::getWeather(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto weather = WeatherRepository::getRecentWeather();
        
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
        error_response["error"] = e.what();
        
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

        if (condition.empty() || visibility_km < 0.0f || wind_speed_kmh < 0.0f)
        {
            Json::Value error_response;
            error_response["error"] = "Invalid weather payload";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        auto weather = WeatherRepository::createWeather(condition, visibility_km, wind_speed_kmh, temperature_c);
        
        Json::Value response;
        response["status"] = "success";
        response["data"] = weather;
        
        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k201Created);
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
