#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class WeatherController : public HttpController<WeatherController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(WeatherController::getWeather, "/weather", Get);
    ADD_METHOD_TO(WeatherController::createWeather, "/weather", Post, "JwtAuthFilter");
    METHOD_LIST_END

    void getWeather(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void createWeather(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
