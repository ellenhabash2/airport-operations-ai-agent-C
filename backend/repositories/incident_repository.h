#pragma once
#include <json/json.h>
#include <string>

class IncidentRepository
{
public:
    static Json::Value getAllIncidents();
    static Json::Value createIncident(const std::string &title, const std::string &description,
                                      const std::string &severity, const std::string &location);
    static Json::Value resolveIncident(const std::string &id);
};
