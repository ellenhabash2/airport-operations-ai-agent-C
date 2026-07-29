#pragma once
#include "PresentationTypes.h"
#include "ToolExecutionRecord.h"
#include <json/json.h>
#include <optional>
#include <vector>

// Deterministic, backend-generated presentation contracts.
//
// The PresentationService turns the trusted structured tool-execution trace of
// a single agent turn into one canonical presentation object for the frontend.
// It is intentionally pure:
//   * it never queries the database or calls repositories/services,
//   * it never calls Gemini and never parses the natural-language answer,
//   * it works only from ToolExecutionRecord data already produced this turn.
//
// Presentation selection is by tool identity and successful structured results,
// following a fixed precedence (operations_overview > gate_assignment >
// runway_status > flight_status > flight_list > incident_list > none). Only
// successful executions contribute data, so a failed mutation can never yield a
// success presentation.
class PresentationService
{
public:
    // Returns the deterministic presentation for this turn, or std::nullopt when
    // no canonical contract applies. The returned value is always validate()-clean.
    static std::optional<Json::Value> generate(const std::vector<ToolExecutionRecord> &executions);

    // Structural validation used before returning or persisting a presentation.
    // Requires a recognized `type` and a `data` object carrying the required
    // fields for that type. Never throws.
    static bool validate(const Json::Value &presentation);

private:
    static Json::Value buildFlightList(const std::vector<ToolExecutionRecord> &executions);
    static Json::Value buildFlightStatus(const std::vector<ToolExecutionRecord> &executions);
    static Json::Value buildGateAssignment(const std::vector<ToolExecutionRecord> &executions);
    static Json::Value buildRunwayStatus(const std::vector<ToolExecutionRecord> &executions);
    static Json::Value buildIncidentList(const std::vector<ToolExecutionRecord> &executions);
    static Json::Value buildOperationsOverview(const std::vector<ToolExecutionRecord> &executions);
};
