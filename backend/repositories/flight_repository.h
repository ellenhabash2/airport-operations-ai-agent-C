#pragma once
#include <json/json.h>
#include <string>

class FlightRepository
{
public:
    static Json::Value getAllFlights();
    static Json::Value getDelayedFlights();

    // Returns { "found": false, "message": ... }
    //      or { "found": true,  "flight": { ... } }
    // Never returns null, so callers (and the agent) can always tell the
    // difference between "no such flight" and a failure.
    static Json::Value getFlightById(const std::string &id);

    // departure_time / arrival_time are required by the schema
    // (NOT NULL + CHECK arrival_time > departure_time).
    static Json::Value createFlight(const std::string &flight_number, const std::string &airline_id,
                                    const std::string &aircraft_id, const std::string &origin,
                                    const std::string &destination,
                                    const std::string &departure_time, const std::string &arrival_time,
                                    const std::string &status);
};