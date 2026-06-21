#pragma once
#include <json/json.h>
#include <string>

class WeatherRepository
{
public:
    static Json::Value getLatestWeather();
    static Json::Value createWeather(const std::string &condition, float visibility_km,
                                     float wind_speed_kmh, float temperature_c);
};
