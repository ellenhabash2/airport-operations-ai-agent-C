#include "terminal_service.h"
#include "domain_error.h"
#include "repositories/terminal_repository.h"
#include <algorithm>
#include <cctype>

namespace
{
std::string trimTerminal(std::string value)
{
    const auto nonspace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), nonspace));
    value.erase(std::find_if(value.rbegin(), value.rend(), nonspace).base(), value.end());
    return value;
}

void validateId(int terminalId)
{
    if (terminalId <= 0)
        throw DomainError(DomainErrorKind::Validation, "invalid_terminal_id", "Terminal ID must be a positive integer");
}
}

TerminalService::TerminalService() : TerminalService({
    TerminalRepository::findAll, TerminalRepository::findById,
    TerminalRepository::findByName, TerminalRepository::findFlightsByTerminal,
    TerminalRepository::getStatus}) {}

TerminalService::TerminalService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}

std::vector<Terminal> TerminalService::getAllTerminals() const { return dependencies_.all(); }

Terminal TerminalService::getTerminalById(int terminalId) const
{
    validateId(terminalId);
    auto terminal = dependencies_.byId(terminalId);
    if (!terminal) throw DomainError(DomainErrorKind::NotFound, "terminal_not_found", "Terminal not found");
    return *terminal;
}

Terminal TerminalService::getTerminalByName(const std::string &name) const
{
    auto cleaned = trimTerminal(name);
    if (cleaned.empty() || cleaned.size() > 80)
        throw DomainError(DomainErrorKind::Validation, "invalid_terminal_name", "Terminal name is invalid");
    auto terminal = dependencies_.byName(cleaned);
    if (!terminal) throw DomainError(DomainErrorKind::NotFound, "terminal_not_found", "Terminal not found");
    return *terminal;
}

TerminalStatus TerminalService::getTerminalStatus(int terminalId) const
{
    getTerminalById(terminalId);
    auto status = dependencies_.status(terminalId);
    if (!status) throw DomainError(DomainErrorKind::NotFound, "terminal_not_found", "Terminal not found");
    return *status;
}

Json::Value TerminalService::getFlightsByTerminal(int terminalId) const
{
    getTerminalById(terminalId);
    return dependencies_.flightsByTerminal(terminalId);
}

