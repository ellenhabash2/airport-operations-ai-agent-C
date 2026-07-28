#include "gate_service.h"
#include "domain_error.h"
#include "repositories/gate_repository.h"
#include <algorithm>
#include <cctype>

namespace { std::string trimGate(std::string value) {
    auto nonspace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), nonspace));
    value.erase(std::find_if(value.rbegin(), value.rend(), nonspace).base(), value.end()); return value;
} }

GateService::GateService() : GateService({
    GateRepository::findAll, GateRepository::findAvailable,
    GateRepository::findByNumber, GateRepository::findById,
    GateRepository::findByTerminal, GateRepository::updateAvailability}) {}
GateService::GateService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value GateService::getAllGates() const { return dependencies_.all(); }
Json::Value GateService::getAvailableGates() const { return dependencies_.available(); }

Json::Value GateService::getGateById(int gateId) const {
    if (gateId <= 0) throw DomainError(DomainErrorKind::Validation, "invalid_gate_id", "Gate ID must be a positive integer");
    auto result = dependencies_.byId(gateId);
    if (!result.get("found", false).asBool()) throw DomainError(DomainErrorKind::NotFound, "gate_not_found", "Gate not found");
    return result["gate"];
}

Json::Value GateService::getGateByNumber(const std::string &gateNumber) const {
    auto number = trimGate(gateNumber);
    if (number.empty() || number.size() > 10) throw DomainError(DomainErrorKind::Validation, "invalid_gate_number", "Gate number is invalid");
    auto result = dependencies_.byNumber(number);
    if (!result.get("found", false).asBool()) throw DomainError(DomainErrorKind::NotFound, "gate_not_found", "Gate not found");
    return result["gate"];
}

Json::Value GateService::getByNumber(const std::string &gateNumber) const { return getGateByNumber(gateNumber); }

Json::Value GateService::getGatesByTerminal(int terminalId) const {
    if (terminalId <= 0) throw DomainError(DomainErrorKind::Validation, "invalid_terminal_id", "Terminal ID must be a positive integer");
    return dependencies_.byTerminal(terminalId);
}

bool GateService::setGateAvailability(int gateId, bool available) const {
    getGateById(gateId);
    return dependencies_.updateAvailability(gateId, available);
}

bool GateService::isGateAvailable(const Json::Value &gate) { return gate.get("status", "").asString() == "AVAILABLE"; }
bool GateService::isGateOperational(const Json::Value &gate) {
    const auto status = gate.get("status", "").asString();
    return status == "AVAILABLE" || status == "OCCUPIED";
}
