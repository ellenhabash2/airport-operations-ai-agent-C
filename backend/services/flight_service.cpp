#include "flight_service.h"
#include "domain_error.h"
#include "repositories/flight_repository.h"
#include <algorithm>
#include <cctype>
#include <charconv>

namespace
{
std::string trim(std::string value)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

int positiveId(const std::string &value, const char *label)
{
    const auto cleaned = trim(value);
    int parsed = 0;
    const auto [end, error] = std::from_chars(cleaned.data(), cleaned.data() + cleaned.size(), parsed);
    if (cleaned.empty() || error != std::errc{} || end != cleaned.data() + cleaned.size() || parsed <= 0)
        throw DomainError(DomainErrorKind::Validation, "invalid_id", std::string(label) + " must be a positive integer");
    return parsed;
}

void normalizeFilter(std::optional<std::string> &value, const char *label)
{
    if (!value) return;
    *value = trim(*value);
    if (value->empty()) { value.reset(); return; }
    if (value->size() > 100)
        throw DomainError(DomainErrorKind::Validation, "invalid_search_filter", std::string(label) + " is too long");
}
}

std::optional<FlightStatus> parseFlightStatus(const std::string &value)
{
    auto normalized = trim(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return std::toupper(c); });
    if (normalized == "SCHEDULED") return FlightStatus::Scheduled;
    if (normalized == "BOARDING") return FlightStatus::Boarding;
    if (normalized == "IN_FLIGHT" || normalized == "DEPARTED") return FlightStatus::InFlight;
    if (normalized == "DELAYED") return FlightStatus::Delayed;
    if (normalized == "CANCELLED") return FlightStatus::Cancelled;
    if (normalized == "LANDED" || normalized == "ARRIVED") return FlightStatus::Landed;
    return std::nullopt;
}

std::string toString(FlightStatus status)
{
    switch (status) {
    case FlightStatus::Scheduled: return "SCHEDULED";
    case FlightStatus::Boarding: return "BOARDING";
    case FlightStatus::InFlight: return "IN_FLIGHT";
    case FlightStatus::Delayed: return "DELAYED";
    case FlightStatus::Cancelled: return "CANCELLED";
    case FlightStatus::Landed: return "LANDED";
    }
    return {};
}

FlightService::FlightService() : FlightService({
    FlightRepository::getAllFlights, FlightRepository::getDelayedFlights,
    FlightRepository::getFlightById, FlightRepository::getFlightByNumber,
    FlightRepository::searchFlights, FlightRepository::updateStatus,
    FlightRepository::assignGateTransactional}) {}

FlightService::FlightService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value FlightService::getAll() const { return dependencies_.all(); }
Json::Value FlightService::getDelayed() const { return dependencies_.delayed(); }

Json::Value FlightService::getById(const std::string &id) const
{
    const int parsed = positiveId(id, "Flight ID");
    auto result = dependencies_.byId(std::to_string(parsed));
    if (!result.get("found", false).asBool())
        throw DomainError(DomainErrorKind::NotFound, "flight_not_found", "Flight not found");
    return result["flight"];
}

Json::Value FlightService::getByNumber(const std::string &flightNumber) const
{
    auto number = trim(flightNumber);
    if (number.empty() || number.size() > 20)
        throw DomainError(DomainErrorKind::Validation, "invalid_flight_number", "Flight number must be between 1 and 20 characters");
    auto result = dependencies_.byNumber(number);
    if (!result.get("found", false).asBool())
        throw DomainError(DomainErrorKind::NotFound, "flight_not_found", "Flight not found");
    return result["flight"];
}

Json::Value FlightService::searchFlights(FlightSearchCriteria criteria) const
{
    normalizeFilter(criteria.origin, "origin");
    normalizeFilter(criteria.destination, "destination");
    normalizeFilter(criteria.airline, "airline");
    if (criteria.status) {
        auto parsed = parseFlightStatus(*criteria.status);
        if (!parsed) throw DomainError(DomainErrorKind::Validation, "invalid_flight_status", "Unsupported flight status");
        criteria.status = toString(*parsed);
    }
    if (criteria.terminalId && *criteria.terminalId <= 0)
        throw DomainError(DomainErrorKind::Validation, "invalid_terminal_id", "Terminal ID must be a positive integer");
    return dependencies_.search(criteria);
}

Json::Value FlightService::updateFlightStatus(const std::string &id, const std::string &status) const
{
    const int flightId = positiveId(id, "Flight ID");
    auto parsed = parseFlightStatus(status);
    if (!parsed) throw DomainError(DomainErrorKind::Validation, "invalid_flight_status", "Unsupported flight status");
    getById(std::to_string(flightId));
    if (!dependencies_.updateStatus(flightId, toString(*parsed)))
        throw DomainError(DomainErrorKind::NotFound, "flight_not_found", "Flight not found");
    return getById(std::to_string(flightId));
}

Json::Value FlightService::assignFlightToGate(const std::string &flightIdValue, const std::string &gateIdValue) const
{
    const int flightId = positiveId(flightIdValue, "Flight ID");
    const int gateId = positiveId(gateIdValue, "Gate ID");
    auto databaseResult = dependencies_.assignGate(flightId, gateId);
    const auto outcome = databaseResult.get("outcome", "error").asString();
    if (outcome == "flight_not_found") throw DomainError(DomainErrorKind::NotFound, "flight_not_found", "Flight not found");
    if (outcome == "gate_not_found") throw DomainError(DomainErrorKind::NotFound, "gate_not_found", "Gate not found");
    if (outcome == "gate_unavailable") throw DomainError(DomainErrorKind::Conflict, "gate_unavailable", "Gate is unavailable");
    if (outcome == "gate_not_operational") throw DomainError(DomainErrorKind::Conflict, "gate_not_operational", "Gate is not operational");
    if (outcome != "success") throw std::runtime_error("Gate assignment failed");
    Json::Value result;
    result["flight"] = getById(std::to_string(flightId));
    result["previous_gate"] = databaseResult["previous_gate"];
    result["new_gate"] = databaseResult["new_gate"];
    return result;
}
