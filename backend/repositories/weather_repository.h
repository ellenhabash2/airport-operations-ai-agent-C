#pragma once
#include <json/json.h>
#include <string>

class WeatherRepository
{
public:
    // The last 10 reports, newest first. Used by GET /weather.
    static Json::Value getRecentWeather();

    // Only the single newest report. Used by the agent's get_latest_weather
    // tool, which must match its name.
    static Json::Value getLatestWeather();

    static Json::Value createWeather(const std::string &condition, float visibility_km,
                                     float wind_speed_kmh, float temperature_c);
};