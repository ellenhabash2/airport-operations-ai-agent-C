#pragma once

#include <json/json.h>
#include <optional>
#include <string>

struct FlightSearchCriteria
{
    std::optional<std::string> origin;
    std::optional<std::string> destination;
    std::optional<std::string> status;
    std::optional<std::string> airline;
    std::optional<int> terminalId;
};

class FlightRepository
{
public:
    static Json::Value getAllFlights();
    static Json::Value getDelayedFlights();
    static Json::Value getFlightById(const std::string &id);
    static Json::Value getFlightByNumber(const std::string &flightNumber);
    static Json::Value searchFlights(const FlightSearchCriteria &criteria);
    static bool updateStatus(int flightId, const std::string &status);
    static Json::Value getFlightsByRunwayId(int runwayId);

    // Locks the flight, target gate, and previous gate and applies every state
    // change in one PostgreSQL transaction. The typed `outcome` lets the
    // service map database state to domain errors without parsing messages.
    static Json::Value assignGateTransactional(int flightId, int gateId);
};