#pragma once
#include <string>

struct Incident
{
    int id{};
    std::string title;
    std::string description;
    std::string severity;
    std::string location;
    std::string status;
    std::string created_at;
};
