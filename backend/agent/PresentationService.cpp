#include "PresentationService.h"
#include <algorithm>
#include <array>
#include <set>
#include <string>

namespace {

// ---- Small helpers over the execution trace -------------------------------

// Latest successful result for a given tool name (by execution sequence), if any.
std::optional<Json::Value> latestSuccess(const std::vector<ToolExecutionRecord> &executions,
                                         const std::string &tool)
{
    std::optional<Json::Value> found;
    for (const auto &record : executions)
        if (record.success && record.tool == tool)
            found = record.result;  // later executions overwrite earlier ones
    return found;
}

// Latest successful result among a set of tool names, tracking the winning
// sequence so precedence within a group is deterministic (highest sequence wins).
std::optional<Json::Value> latestSuccessAmong(const std::vector<ToolExecutionRecord> &executions,
                                              const std::set<std::string> &tools)
{
    std::optional<Json::Value> found;
    long long bestSequence = -1;
    for (const auto &record : executions) {
        if (!record.success || tools.count(record.tool) == 0) continue;
        if (static_cast<long long>(record.sequence) >= bestSequence) {
            bestSequence = static_cast<long long>(record.sequence);
            found = record.result;
        }
    }
    return found;
}

bool isValidObject(const Json::Value &value) { return value.isObject() && !value.isMember("error"); }

// Append flight/incident entities from a successful list result, de-duplicating
// by stable `id` so repeated list calls never create duplicate cards.
void mergeEntities(const Json::Value &source, Json::Value &target, std::set<int> &seenIds)
{
    if (!source.isArray()) return;
    for (const auto &entity : source) {
        if (!entity.isObject()) continue;
        if (entity.isMember("id") && entity["id"].isIntegral()) {
            const int id = entity["id"].asInt();
            if (seenIds.count(id)) continue;
            seenIds.insert(id);
        }
        target.append(entity);
    }
}

Json::Value wrap(PresentationType type, Json::Value data)
{
    Json::Value presentation;
    presentation["type"] = toString(type);
    presentation["data"] = std::move(data);
    return presentation;
}

const std::set<std::string> kFlightListTools{
    "get_all_flights", "find_delayed_flights", "search_flights", "get_flights_by_terminal"};
const std::set<std::string> kFlightStatusTools{
    "get_flight_by_id", "get_flight_by_number", "get_flight_details", "update_flight_status"};
const std::set<std::string> kRunwayReadTools{
    "get_runway_status", "get_runway_by_id", "get_runway_by_code"};
const std::set<std::string> kIncidentListTools{
    "get_all_incidents", "get_active_incidents", "get_incidents_by_severity", "search_incidents"};

} // namespace

// ---- Contract builders -----------------------------------------------------

Json::Value PresentationService::buildFlightList(const std::vector<ToolExecutionRecord> &executions)
{
    Json::Value flights(Json::arrayValue);
    std::set<int> seen;
    bool matched = false;
    for (const auto &record : executions) {
        if (!record.success || kFlightListTools.count(record.tool) == 0) continue;
        if (!record.result.isArray()) continue;  // fail safe on malformed shape
        matched = true;
        mergeEntities(record.result, flights, seen);
    }
    if (!matched) return Json::nullValue;
    Json::Value data;
    data["flights"] = flights;  // may legitimately be an empty array
    return wrap(PresentationType::FlightList, data);
}

Json::Value PresentationService::buildFlightStatus(const std::vector<ToolExecutionRecord> &executions)
{
    const auto flight = latestSuccessAmong(executions, kFlightStatusTools);
    if (!flight || !isValidObject(*flight)) return Json::nullValue;
    Json::Value data;
    data["flight"] = *flight;
    return wrap(PresentationType::FlightStatus, data);
}

Json::Value PresentationService::buildGateAssignment(const std::vector<ToolExecutionRecord> &executions)
{
    const auto assignment = latestSuccess(executions, "assign_flight_to_gate");
    if (!assignment || !isValidObject(*assignment)) return Json::nullValue;
    if (!(*assignment)["flight"].isObject() || !(*assignment)["new_gate"].isObject()) return Json::nullValue;
    Json::Value data;
    data["flight"] = (*assignment)["flight"];
    data["previous_gate"] = (*assignment).isMember("previous_gate") && (*assignment)["previous_gate"].isObject()
                                ? (*assignment)["previous_gate"]
                                : Json::Value(Json::nullValue);
    data["new_gate"] = (*assignment)["new_gate"];
    return wrap(PresentationType::GateAssignment, data);
}

Json::Value PresentationService::buildRunwayStatus(const std::vector<ToolExecutionRecord> &executions)
{
    Json::Value runways(Json::arrayValue);
    Json::Value affected(Json::arrayValue);

    // A mutation result carries both the updated runway and the affected flights,
    // so it takes precedence over a plain read when present.
    if (const auto update = latestSuccess(executions, "update_runway_status")) {
        if (isValidObject(*update) && (*update)["runway"].isObject()) {
            runways.append((*update)["runway"]);
            if ((*update)["affected_flights"].isArray()) affected = (*update)["affected_flights"];
            Json::Value data;
            data["runways"] = runways;
            data["affected_flights"] = affected;
            return wrap(PresentationType::RunwayStatus, data);
        }
    }

    const auto read = latestSuccessAmong(executions, kRunwayReadTools);
    if (!read) return Json::nullValue;
    if (read->isArray()) runways = *read;              // get_runway_status -> list
    else if (isValidObject(*read)) runways.append(*read);  // by_id / by_code -> single
    else return Json::nullValue;
    Json::Value data;
    data["runways"] = runways;
    data["affected_flights"] = affected;  // reads never imply affected flights
    return wrap(PresentationType::RunwayStatus, data);
}

Json::Value PresentationService::buildIncidentList(const std::vector<ToolExecutionRecord> &executions)
{
    Json::Value incidents(Json::arrayValue);
    std::set<int> seen;
    bool matched = false;
    for (const auto &record : executions) {
        if (!record.success) continue;
        const bool isList = kIncidentListTools.count(record.tool) > 0;
        const bool isMutation = record.tool == "create_incident" || record.tool == "resolve_incident";
        if (!isList && !isMutation) continue;
        if (isList && record.result.isArray()) { matched = true; mergeEntities(record.result, incidents, seen); }
        else if (isMutation && isValidObject(record.result)) {
            matched = true;
            Json::Value single(Json::arrayValue); single.append(record.result);
            mergeEntities(single, incidents, seen);
        }
    }
    if (!matched) return Json::nullValue;
    Json::Value data;
    data["incidents"] = incidents;
    return wrap(PresentationType::IncidentList, data);
}

Json::Value PresentationService::buildOperationsOverview(const std::vector<ToolExecutionRecord> &executions)
{
    const auto delayed = latestSuccess(executions, "find_delayed_flights");
    const auto incidents = latestSuccess(executions, "get_active_incidents");
    const auto weather = latestSuccess(executions, "get_latest_weather");
    // Documented rule: the overview requires ALL THREE required sources to have
    // succeeded. Otherwise we fall through to the strongest single-domain
    // presentation via normal precedence.
    if (!delayed || !incidents || !weather) return Json::nullValue;
    if (!delayed->isArray() || !incidents->isArray() || !isValidObject(*weather)) return Json::nullValue;
    Json::Value data;
    data["delayed_flights"] = *delayed;
    data["active_incidents"] = *incidents;
    data["weather"] = *weather;
    return wrap(PresentationType::OperationsOverview, data);
}

// ---- Public entry points ---------------------------------------------------

std::optional<Json::Value> PresentationService::generate(const std::vector<ToolExecutionRecord> &executions)
{
    // Fixed precedence. First valid contract wins; exactly one is returned.
    const std::array<Json::Value (*)(const std::vector<ToolExecutionRecord> &), 6> builders{
        &PresentationService::buildOperationsOverview,
        &PresentationService::buildGateAssignment,
        &PresentationService::buildRunwayStatus,
        &PresentationService::buildFlightStatus,
        &PresentationService::buildFlightList,
        &PresentationService::buildIncidentList};
    for (const auto builder : builders) {
        Json::Value candidate = builder(executions);
        if (!candidate.isNull() && validate(candidate)) return candidate;
    }
    return std::nullopt;
}

bool PresentationService::validate(const Json::Value &presentation)
{
    if (!presentation.isObject()) return false;
    if (!presentation.isMember("type") || !presentation["type"].isString()) return false;
    const auto type = presentationTypeFromString(presentation["type"].asString());
    if (!type) return false;
    if (!presentation.isMember("data") || !presentation["data"].isObject()) return false;
    const Json::Value &data = presentation["data"];
    switch (*type) {
    case PresentationType::FlightList:
        return data["flights"].isArray();
    case PresentationType::FlightStatus:
        return data["flight"].isObject();
    case PresentationType::GateAssignment:
        return data["flight"].isObject() && data["new_gate"].isObject() &&
               (data["previous_gate"].isObject() || data["previous_gate"].isNull());
    case PresentationType::RunwayStatus:
        return data["runways"].isArray() && data["affected_flights"].isArray();
    case PresentationType::IncidentList:
        return data["incidents"].isArray();
    case PresentationType::OperationsOverview:
        return data["delayed_flights"].isArray() && data["active_incidents"].isArray() &&
               (data["weather"].isObject() || data["weather"].isNull());
    }
    return false;
}
