#include "runway_service.h"
#include "domain_error.h"
#include "repositories/flight_repository.h"
#include "repositories/runway_repository.h"
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
}

std::optional<RunwayStatus> parseRunwayStatus(const std::string &value)
{
    auto normalized = trim(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    if (normalized == "OPERATIONAL" || normalized == "AVAILABLE" || normalized == "OPEN")
        return RunwayStatus::Operational;
    if (normalized == "MAINTENANCE") return RunwayStatus::Maintenance;
    if (normalized == "CLOSED") return RunwayStatus::Closed;
    return std::nullopt;
}

std::string toString(RunwayStatus status)
{
    switch (status) {
    case RunwayStatus::Operational: return "OPERATIONAL";
    case RunwayStatus::Maintenance: return "MAINTENANCE";
    case RunwayStatus::Closed: return "CLOSED";
    }
    return {};
}

RunwayService::RunwayService()
    : RunwayService(Dependencies{
          RunwayRepository::getAllRunways,
          RunwayRepository::getRunwayById,
          RunwayRepository::getRunwayByCode,
          RunwayRepository::updateStatus,
          FlightRepository::getFlightsByRunwayId}) {}

RunwayService::RunwayService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}

RunwayService::RunwayService(std::function<Json::Value()> all)
    : dependencies_{std::move(all), nullptr, nullptr, nullptr, nullptr} {}

Json::Value RunwayService::getStatus() const { return dependencies_.all(); }

Json::Value RunwayService::getById(const std::string &id) const
{
    const int runwayId = positiveId(id, "Runway ID");
    auto result = dependencies_.byId(runwayId);
    if (!result.get("found", false).asBool())
        throw DomainError(DomainErrorKind::NotFound, "runway_not_found", "Runway not found");
    return result["runway"];
}

Json::Value RunwayService::getByCode(const std::string &runwayCode) const
{
    auto code = trim(runwayCode);
    if (code.empty() || code.size() > 10)
        throw DomainError(DomainErrorKind::Validation, "invalid_runway_code",
                          "Runway code must be between 1 and 10 characters");
    auto result = dependencies_.byCode(code);
    if (!result.get("found", false).asBool())
        throw DomainError(DomainErrorKind::NotFound, "runway_not_found", "Runway not found");
    return result["runway"];
}

Json::Value RunwayService::updateStatus(const std::string &id, const std::string &status) const
{
    auto runway = getById(id);
    return applyStatusUpdate(runway, status);
}

Json::Value RunwayService::updateStatusByCode(const std::string &runwayCode, const std::string &status) const
{
    auto runway = getByCode(runwayCode);
    return applyStatusUpdate(runway, status);
}

Json::Value RunwayService::applyStatusUpdate(const Json::Value &runway, const std::string &status) const
{
    auto parsed = parseRunwayStatus(status);
    if (!parsed)
        throw DomainError(DomainErrorKind::Validation, "invalid_runway_status",
                          "Unsupported runway status. Allowed values are: OPERATIONAL, MAINTENANCE, CLOSED");

    const std::string normalized = toString(*parsed);
    const int runwayId = runway["id"].asInt();
    const std::string previousStatus = runway["status"].asString();

    if (!dependencies_.update(runwayId, normalized))
        throw DomainError(DomainErrorKind::NotFound, "runway_not_found", "Runway not found");

    Json::Value updatedRunway = runway;
    updatedRunway["status"] = normalized;

    Json::Value affected = dependencies_.affectedFlights(runwayId);
    Json::Value affectedList(Json::arrayValue);
    for (const auto &flight : affected) {
        Json::Value item;
        item["flight_number"] = flight["flight_number"];
        item["status"] = flight["status"];
        item["origin"] = flight["origin"];
        item["destination"] = flight["destination"];
        affectedList.append(item);
    }

    Json::Value result;
    result["updated"] = true;
    result["previous_status"] = previousStatus;
    result["runway"] = updatedRunway;
    result["affected_flight_count"] = static_cast<int>(affectedList.size());
    result["affected_flights"] = affectedList;
    return result;
}