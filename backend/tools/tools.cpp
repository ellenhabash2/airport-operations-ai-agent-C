#include "tools.h"
#include "repositories/flight_repository.h"
#include "repositories/incident_repository.h"
#include "repositories/gate_repository.h"
#include "repositories/runway_repository.h"
#include "repositories/weather_repository.h"
#include <array>
#include <algorithm>

namespace
{
// severity must match the incidents_severity_check CHECK constraint in the
// schema. The REST endpoint already validates this, but the agent's
// create_incident tool passes the model's raw value straight to PostgreSQL,
// so an invalid severity (e.g. "VERY_HIGH") would fail the CHECK and turn the
// whole /agent/query request into a 500. Validate here instead.
bool isValidSeverity(const std::string &severity)
{
    static constexpr std::array<const char *, 4> allowed = {"LOW", "MEDIUM", "HIGH", "CRITICAL"};
    return std::find(allowed.begin(), allowed.end(), severity) != allowed.end();
}
}

// ---- Read tools ----

// Returns all flights currently delayed.
Json::Value Tools::find_delayed_flights()
{
    return FlightRepository::getDelayedFlights();
}

// Returns airport operational incidents.
Json::Value Tools::get_active_incidents()
{
    // Only incidents that still need attention (OPEN or INVESTIGATING),
    // matching the tool name the model sees.
    return IncidentRepository::getActiveIncidents();
}

// Returns all flights.
Json::Value Tools::get_all_flights()
{
    return FlightRepository::getAllFlights();
}

// Returns full details of a single flight by its id.
Json::Value Tools::get_flight_details(const std::string &id)
{
    return FlightRepository::getFlightById(id);
}

// Returns all gates and their status.
Json::Value Tools::get_available_gates()
{
    // Only gates whose status is AVAILABLE, matching the tool name.
    return GateRepository::getAvailableGates();
}

// Returns all runways and their status.
Json::Value Tools::get_runway_status()
{
    return RunwayRepository::getAllRunways();
}

// Returns the latest weather report.
Json::Value Tools::get_latest_weather()
{
    return WeatherRepository::getLatestWeather();
}

// ---- Action tools ----

// Resolves an incident by id (applies the already-resolved business rule).
Json::Value Tools::resolve_incident(const std::string &id)
{
    return IncidentRepository::resolveIncident(id);
}

// Creates a new incident.
Json::Value Tools::create_incident(const std::string &title, const std::string &description,
                                   const std::string &severity, const std::string &location)
{
    // Validate before touching the database so a bad value from the model
    // becomes a clear tool error the agent can explain, not a 500.
    if (title.empty() || description.empty())
    {
        Json::Value err;
        err["error"] = "title and description are required and must not be empty.";
        return err;
    }
    if (!isValidSeverity(severity))
    {
        Json::Value err;
        err["error"] = "severity must be one of: LOW, MEDIUM, HIGH, CRITICAL.";
        return err;
    }

    return IncidentRepository::createIncident(title, description, severity, location);
}
