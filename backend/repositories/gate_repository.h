#pragma once
#include <json/json.h>
#include <string>

class GateRepository
{
public:
    static Json::Value findAll();
    static Json::Value findAvailable();
    static Json::Value findById(int gateId);
    static Json::Value findByNumber(const std::string &gateNumber);
    static Json::Value findByTerminal(int terminalId);
    static bool updateAvailability(int gateId, bool available);
};
