#pragma once
#include <string>

struct Aircraft
{
    int id{};
    std::string registration_number;
    std::string aircraft_type;
    int airline_id{};
    std::string status;
};
