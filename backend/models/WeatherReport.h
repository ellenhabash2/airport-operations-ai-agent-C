#pragma once
#include <string>

struct WeatherReport
{
    int id{};
    std::string condition;
    double visibility_km{};
    double wind_speed_kmh{};
    std::string wind_direction;
    double temperature_c{};
    double pressure_hpa{};
    std::string created_at;
};
