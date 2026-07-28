#pragma once

#include "models/terminal.h"
#include <json/json.h>
#include <optional>
#include <string>
#include <vector>

class TerminalRepository
{
public:
    static std::vector<Terminal> findAll();
    static std::optional<Terminal> findById(int terminalId);
    static std::optional<Terminal> findByName(const std::string &name);
    static Json::Value findFlightsByTerminal(int terminalId);
    static std::optional<TerminalStatus> getStatus(int terminalId);
};

