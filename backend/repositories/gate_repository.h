#pragma once
#include <json/json.h>
#include <string>

class GateRepository
{
public:
    // Every gate, whatever its status. Used by GET /gates.
    static Json::Value getAllGates();

    // Only gates that are free right now. Used by the agent's
    // get_available_gates tool, which must match its name.
    static Json::Value getAvailableGates();
    static Json::Value getGateByNumber(const std::string &gateNumber);
};
