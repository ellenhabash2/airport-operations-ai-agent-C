#pragma once
#include <string>

struct Runway
{
    int id{};
    std::string runway_code;
    std::string status;
    int length_meters{};
    std::string surface;
};
