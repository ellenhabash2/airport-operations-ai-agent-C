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
};