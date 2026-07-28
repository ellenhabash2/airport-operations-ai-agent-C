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

GateService::GateService() : GateService({GateRepository::getAllGates, GateRepository::getAvailableGates, GateRepository::getGateByNumber}) {}
GateService::GateService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value GateService::getAll() const { return dependencies_.all(); }
Json::Value GateService::getAvailable() const { return dependencies_.available(); }
Json::Value GateService::getByNumber(const std::string &gateNumber) const {
    auto number = trimGate(gateNumber);
    if (number.empty() || number.size() > 10) throw DomainError(DomainErrorKind::Validation, "invalid_gate_number", "Gate number is invalid");
    auto result = dependencies_.byNumber(number);
    if (!result.get("found", false).asBool()) throw DomainError(DomainErrorKind::NotFound, "gate_not_found", "Gate not found");
    return result["gate"];
}
