#pragma once

#include "models/terminal.h"
#include <functional>
#include <json/json.h>
#include <optional>
#include <string>
#include <vector>

class TerminalService
{
public:
    struct Dependencies
    {
        std::function<std::vector<Terminal>()> all;
        std::function<std::optional<Terminal>(int)> byId;
        std::function<std::optional<Terminal>(const std::string &)> byName;
        std::function<Json::Value(int)> flightsByTerminal;
        std::function<std::optional<TerminalStatus>(int)> status;
    };

    TerminalService();
    explicit TerminalService(Dependencies dependencies);
    std::vector<Terminal> getAllTerminals() const;
    Terminal getTerminalById(int terminalId) const;
    Terminal getTerminalByName(const std::string &name) const;
    TerminalStatus getTerminalStatus(int terminalId) const;
    Json::Value getFlightsByTerminal(int terminalId) const;

private:
    Dependencies dependencies_;
};

