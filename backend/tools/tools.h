#pragma once
#include <json/json.h>
#include <string>

// AeroMind AI Agent function tools.
// Each static method is one tool the agent can call.
// Tools wrap the repository layer and return JSON results.
class Tools
{
public:
    // Read tools
    static Json::Value find_delayed_flights();
    static Json::Value get_active_incidents();
};
