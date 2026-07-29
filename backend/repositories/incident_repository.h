#pragma once
#include <json/json.h>
#include <string>

class IncidentRepository
{
public:
    // Every incident, including RESOLVED ones. Used by GET /incidents.
    static Json::Value getAllIncidents();

    // Only incidents that still need attention (OPEN or INVESTIGATING).
    // Used by the agent's get_active_incidents tool, which must match its name.
    static Json::Value getActiveIncidents();
    static Json::Value getIncidentsBySeverity(const std::string &severity);
    static Json::Value searchIncidents(const std::string &query);
    static Json::Value getIncidentById(const std::string &id);

    static Json::Value createIncident(const std::string &title, const std::string &description,
                                      const std::string &severity, const std::string &location);
    static Json::Value resolveIncident(const std::string &id);
};
