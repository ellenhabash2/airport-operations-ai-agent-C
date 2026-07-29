#pragma once
#include <optional>
#include <string>

// Canonical, frontend-facing presentation contract identifiers.
//
// These strings are a stable public API. They intentionally do NOT expose any
// internal C++ class names. Serialization is strict: unknown input never
// silently maps to an arbitrary type.
enum class PresentationType
{
    FlightList,
    FlightStatus,
    GateAssignment,
    RunwayStatus,
    IncidentList,
    OperationsOverview
};

inline std::string toString(PresentationType type)
{
    switch (type) {
    case PresentationType::FlightList: return "flight_list";
    case PresentationType::FlightStatus: return "flight_status";
    case PresentationType::GateAssignment: return "gate_assignment";
    case PresentationType::RunwayStatus: return "runway_status";
    case PresentationType::IncidentList: return "incident_list";
    case PresentationType::OperationsOverview: return "operations_overview";
    }
    return {};
}

inline std::optional<PresentationType> presentationTypeFromString(const std::string &value)
{
    if (value == "flight_list") return PresentationType::FlightList;
    if (value == "flight_status") return PresentationType::FlightStatus;
    if (value == "gate_assignment") return PresentationType::GateAssignment;
    if (value == "runway_status") return PresentationType::RunwayStatus;
    if (value == "incident_list") return PresentationType::IncidentList;
    if (value == "operations_overview") return PresentationType::OperationsOverview;
    return std::nullopt;
}
