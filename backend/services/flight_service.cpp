#include "flight_service.h"
#include "domain_error.h"
#include "repositories/flight_repository.h"
#include <algorithm>
#include <cctype>

FlightService::FlightService() : FlightService({FlightRepository::getAllFlights,
    FlightRepository::getDelayedFlights, FlightRepository::getFlightById}) {}
FlightService::FlightService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value FlightService::getAll() const { return dependencies_.all(); }
Json::Value FlightService::getDelayed() const { return dependencies_.delayed(); }
Json::Value FlightService::getById(const std::string &id) const {
    if (id.empty() || !std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isdigit(c); }))
        throw DomainError(DomainErrorKind::Validation, "invalid_flight_id", "Flight ID must be a positive integer");
    auto result = dependencies_.byId(id);
    if (!result.get("found", false).asBool())
        throw DomainError(DomainErrorKind::NotFound, "flight_not_found", "Flight not found");
    return result["flight"];
}
