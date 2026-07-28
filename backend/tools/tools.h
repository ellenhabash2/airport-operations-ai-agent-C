#pragma once
#include <json/json.h>
#include <string>

// AeroMind AI Agent function tools.
// Each static method is one tool the agent can call.
// Tools adapt Gemini arguments/results around shared domain services.
class Tools
{
public:
    // ---- Read tools ----
    static Json::Value find_delayed_flights();
    static Json::Value get_active_incidents();
    static Json::Value get_all_flights();
    static Json::Value get_flight_details(const std::string &id);
    static Json::Value get_flight_by_id(const std::string &id);
    static Json::Value get_flight_by_number(const std::string &flightNumber);
    static Json::Value search_flights(const Json::Value &arguments);
    static Json::Value get_available_gates();
    static Json::Value get_runway_status();
    static Json::Value get_latest_weather();

    // ---- Action tools ----
    static Json::Value resolve_incident(const std::string &id);
    static Json::Value create_incident(const std::string &title, const std::string &description,
                                       const std::string &severity, const std::string &location);
    static Json::Value update_flight_status(const Json::Value &arguments);
    static Json::Value assign_flight_to_gate(const Json::Value &arguments);
};
