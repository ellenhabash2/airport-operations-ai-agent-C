#include "tools.h"
#include "repositories/flight_repository.h"
#include "repositories/incident_repository.h"
#include "repositories/gate_repository.h"
#include "repositories/runway_repository.h"
#include "repositories/weather_repository.h"

// ---- Read tools ----

// Returns all flights currently delayed.
Json::Value Tools::find_delayed_flights()
{
    return FlightRepository::getDelayedFlights();
}

// Returns airport operational incidents.
Json::Value Tools::get_active_incidents()
{
    return IncidentRepository::getAllIncidents();
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
    return GateRepository::getAllGates();
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
    return IncidentRepository::createIncident(title, description, severity, location);
}
