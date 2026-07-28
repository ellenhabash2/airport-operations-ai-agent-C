#pragma once

#include "repositories/flight_repository.h"
#include <functional>
#include <json/json.h>
#include <optional>
#include <string>

enum class FlightStatus { Scheduled, Boarding, InFlight, Delayed, Cancelled, Landed };
std::optional<FlightStatus> parseFlightStatus(const std::string &value);
std::string toString(FlightStatus status);

class FlightService
{
public:
    struct Dependencies {
        std::function<Json::Value()> all;
        std::function<Json::Value()> delayed;
        std::function<Json::Value(const std::string &)> byId;
        std::function<Json::Value(const std::string &)> byNumber;
        std::function<Json::Value(const FlightSearchCriteria &)> search;
        std::function<bool(int, const std::string &)> updateStatus;
        std::function<Json::Value(int, int)> assignGate;
    };

    FlightService();
    explicit FlightService(Dependencies dependencies);
    Json::Value getAll() const;
    Json::Value getDelayed() const;
    Json::Value getById(const std::string &id) const;
    Json::Value getByNumber(const std::string &flightNumber) const;
    Json::Value searchFlights(FlightSearchCriteria criteria) const;
    Json::Value updateFlightStatus(const std::string &id, const std::string &status) const;
    Json::Value assignFlightToGate(const std::string &flightId, const std::string &gateId) const;

private:
    Dependencies dependencies_;
};
