#include "tools.h"
#include "repositories/flight_repository.h"
#include "repositories/incident_repository.h"

// Tool: find_delayed_flights
// Returns all flights currently delayed.
Json::Value Tools::find_delayed_flights()
{
    return FlightRepository::getDelayedFlights();
}

// Tool: get_active_incidents
// Returns airport operational incidents.
Json::Value Tools::get_active_incidents()
{
    return IncidentRepository::getAllIncidents();
}
