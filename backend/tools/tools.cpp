#include "tools.h"
#include "services/domain_error.h"
#include "services/flight_service.h"
#include "services/gate_service.h"
#include "services/incident_service.h"
#include "services/runway_service.h"
#include "services/weather_service.h"

namespace {
template <typename Operation> Json::Value safely(Operation operation) {
    try { return operation(); }
    catch (const DomainError &error) { Json::Value value; value["error"] = error.what(); value["code"] = error.code(); return value; }
}
}

// ---- Read tools ----

// Returns all flights currently delayed.
Json::Value Tools::find_delayed_flights()
{
    return safely([] { return FlightService{}.getDelayed(); });
}

// Returns airport operational incidents.
Json::Value Tools::get_active_incidents()
{
    // Only incidents that still need attention (OPEN or INVESTIGATING),
    // matching the tool name the model sees.
    return safely([] { return IncidentService{}.getActive(); });
}

// Returns all flights.
Json::Value Tools::get_all_flights()
{
    return safely([] { return FlightService{}.getAll(); });
}

// Returns full details of a single flight by its id.
Json::Value Tools::get_flight_details(const std::string &id)
{
    try { Json::Value result; result["found"] = true; result["flight"] = FlightService{}.getById(id); return result; }
    catch (const DomainError &error) {
        Json::Value result;
        if (error.kind() == DomainErrorKind::NotFound || error.kind() == DomainErrorKind::Validation) {
            result["found"] = false;
            result["message"] = error.kind() == DomainErrorKind::Validation
                ? "Flight id must be a positive integer." : "No flight exists with id " + id + ".";
            return result;
        }
        result["error"] = error.what(); result["code"] = error.code(); return result;
    }
}

// Returns all gates and their status.
Json::Value Tools::get_available_gates()
{
    // Only gates whose status is AVAILABLE, matching the tool name.
    return safely([] { return GateService{}.getAvailable(); });
}

// Returns all runways and their status.
Json::Value Tools::get_runway_status()
{
    return safely([] { return RunwayService{}.getStatus(); });
}

// Returns the latest weather report.
Json::Value Tools::get_latest_weather()
{
    return safely([] { return WeatherService{}.getLatest(); });
}

// ---- Action tools ----

// Resolves an incident by id (applies the already-resolved business rule).
Json::Value Tools::resolve_incident(const std::string &id)
{
    try { Json::Value result; result["found"] = true; result["already_resolved"] = false;
        result["incident"] = IncidentService{}.resolve(id); return result; }
    catch (const DomainError &error) {
        Json::Value result;
        if (error.kind() == DomainErrorKind::NotFound || error.kind() == DomainErrorKind::Validation) { result["found"] = false; return result; }
        if (error.kind() == DomainErrorKind::Conflict) { result["found"] = true; result["already_resolved"] = true; return result; }
        result["error"] = error.what(); result["code"] = error.code(); return result;
    }
}

// Creates a new incident.
Json::Value Tools::create_incident(const std::string &title, const std::string &description,
                                   const std::string &severity, const std::string &location)
{
    return safely([&] { return IncidentService{}.create(title, description, severity, location); });
}
