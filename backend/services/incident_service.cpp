#include "incident_service.h"
#include "domain_error.h"
#include "repositories/incident_repository.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace {
bool positiveInteger(const std::string &value) { return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }); }
bool validSeverity(const std::string &value) {
    static constexpr std::array<const char *, 4> values{"LOW", "MEDIUM", "HIGH", "CRITICAL"};
    return std::find(values.begin(), values.end(), value) != values.end();
}
}
IncidentService::IncidentService() : IncidentService({IncidentRepository::getAllIncidents,
    IncidentRepository::getActiveIncidents, IncidentRepository::createIncident, IncidentRepository::resolveIncident}) {}
IncidentService::IncidentService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value IncidentService::getAll() const { return dependencies_.all(); }
Json::Value IncidentService::getActive() const { return dependencies_.active(); }
Json::Value IncidentService::create(const std::string &title, const std::string &description,
                                    const std::string &severity, const std::string &location) const {
    if (title.empty() || description.empty() || !validSeverity(severity))
        throw DomainError(DomainErrorKind::Validation, "invalid_incident", "Invalid incident payload");
    return dependencies_.create(title, description, severity, location);
}
Json::Value IncidentService::resolve(const std::string &id) const {
    if (!positiveInteger(id))
        throw DomainError(DomainErrorKind::Validation, "invalid_incident_id", "Incident ID must be a positive integer");
    auto result = dependencies_.resolve(id);
    if (!result.get("found", false).asBool())
        throw DomainError(DomainErrorKind::NotFound, "incident_not_found", "Incident not found");
    if (result.get("already_resolved", false).asBool())
        throw DomainError(DomainErrorKind::Conflict, "incident_already_resolved", "Incident is already resolved");
    return result["incident"];
}
