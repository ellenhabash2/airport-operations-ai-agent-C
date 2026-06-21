#pragma once
#include <string>

struct Flight
{
    int id{};
    std::string flight_number;
    int airline_id{};
    int aircraft_id{};
    int gate_id{};
    int runway_id{};
    std::string origin;
    std::string destination;
    std::string departure_time;
    std::string arrival_time;
    std::string status;
};
