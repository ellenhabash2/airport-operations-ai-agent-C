#pragma once
#include <json/json.h>
#include <string>
#include <vector>

class FlightRepository
{
public:
    static Json::Value getAllFlights();
    static Json::Value getDelayedFlights();
    static Json::Value getFlightById(const std::string &id);
    static Json::Value createFlight(const std::string &flight_number, const std::string &airline_id,
                                    const std::string &aircraft_id, const std::string &origin,
                                    const std::string &destination, const std::string &status);
};
