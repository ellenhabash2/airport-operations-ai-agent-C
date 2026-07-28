#pragma once

#include <json/json.h>
#include <optional>
#include <string>

struct Terminal
{
    int id{};
    std::string name;
    std::string code;
    int capacity{};
};

struct TerminalStatus
{
    Terminal terminal;
    int totalGates{};
    int availableGates{};
    int occupiedGates{};
    int nonOperationalGates{};
    int activeFlights{};
};

inline Json::Value terminalToJson(const Terminal &terminal)
{
    Json::Value value;
    value["id"] = terminal.id;
    value["name"] = terminal.name;
    value["code"] = terminal.code;
    value["capacity"] = terminal.capacity;
    return value;
}

inline Json::Value terminalStatusToJson(const TerminalStatus &status)
{
    Json::Value value;
    value["terminal"] = terminalToJson(status.terminal);
    value["total_gates"] = status.totalGates;
    value["available_gates"] = status.availableGates;
    value["occupied_gates"] = status.occupiedGates;
    value["non_operational_gates"] = status.nonOperationalGates;
    value["active_flights"] = status.activeFlights;
    return value;
}

