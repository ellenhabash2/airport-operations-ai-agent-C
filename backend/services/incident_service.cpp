#include "incident_service.h"
#include "domain_error.h"
#include "repositories/incident_repository.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>

namespace {
bool positiveInteger(const std::string &value) {
    return !value.empty() &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }) &&
        std::any_of(value.begin(), value.end(), [](char c) { return c != '0'; });
}
std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
std::string canonicalSeverity(const std::string &value) {
    auto normalized = trim(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    static constexpr std::array<const char *, 4> values{"LOW", "MEDIUM", "HIGH", "CRITICAL"};
    return std::find(values.begin(), values.end(), normalized) != values.end() ? normalized : "";
}
}
IncidentService::IncidentService() : IncidentService({IncidentRepository::getAllIncidents,
    IncidentRepository::getActiveIncidents, IncidentRepository::getIncidentsBySeverity,
    IncidentRepository::searchIncidents, IncidentRepository::getIncidentById,
    IncidentRepository::createIncident, IncidentRepository::resolveIncident}) {}
IncidentService::IncidentService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value IncidentService::getAll() const { return dependencies_.all(); }
Json::Value IncidentService::getActive() const { return dependencies_.active(); }
Json::Value IncidentService::getBySeverity(const std::string &severity) const {
    const auto canonical = canonicalSeverity(severity);
    if (canonical.empty()) throw DomainError(DomainErrorKind::Validation, "invalid_incident_severity", "Invalid incident severity");
    return dependencies_.bySeverity(canonical);
}
Json::Value IncidentService::search(const std::string &query) const {
    const auto normalized = trim(query);
    if (normalized.empty() || normalized.size() > 200)
        throw DomainError(DomainErrorKind::Validation, "invalid_incident_search", "Search query must contain 1 to 200 characters");
    return dependencies_.search(normalized);
}
Json::Value IncidentService::getById(const std::string &id) const {
    if (!positiveInteger(id)) throw DomainError(DomainErrorKind::Validation, "invalid_incident_id", "Incident ID must be a positive integer");
    auto incident = dependencies_.byId(id);
    if (incident.isNull() || incident.empty()) throw DomainError(DomainErrorKind::NotFound, "incident_not_found", "Incident not found");
    return incident;
}
Json::Value IncidentService::create(const std::string &title, const std::string &description,
                                    const std::string &severity, const std::string &location) const {
    const auto normalizedTitle = trim(title), normalizedDescription = trim(description);
    const auto canonical = canonicalSeverity(severity);
    if (normalizedTitle.empty() || normalizedTitle.size() > 200 || normalizedDescription.empty() ||
        normalizedDescription.size() > 5000 || location.size() > 150 || canonical.empty())
        throw DomainError(DomainErrorKind::Validation, "invalid_incident", "Invalid incident payload");
    return dependencies_.create(normalizedTitle, normalizedDescription, canonical, trim(location));
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
