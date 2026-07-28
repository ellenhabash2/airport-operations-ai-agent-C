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
    static Json::Value get_available_gates();
    static Json::Value get_runway_status();
    static Json::Value get_latest_weather();

    // ---- Action tools ----
    static Json::Value resolve_incident(const std::string &id);
    static Json::Value create_incident(const std::string &title, const std::string &description,
                                       const std::string &severity, const std::string &location);
};
