#pragma once
#include <string>

struct Airline
{
    int id{};
    std::string name;
    std::string iata_code;
    std::string icao_code;
    std::string country;
};
