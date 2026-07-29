#include "tools.h"
#include "services/domain_error.h"
#include "services/flight_service.h"
#include "services/gate_service.h"
#include "services/incident_service.h"
#include "services/runway_service.h"
#include "services/terminal_service.h"
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

Json::Value Tools::get_flight_by_id(const std::string &id)
{
    return safely([&] { return FlightService{}.getById(id); });
}

Json::Value Tools::get_flight_by_number(const std::string &flightNumber)
{
    return safely([&] { return FlightService{}.getByNumber(flightNumber); });
}

Json::Value Tools::search_flights(const Json::Value &arguments)
{
    return safely([&] {
        FlightSearchCriteria criteria;
        auto text = [&](const char *name, std::optional<std::string> &field) {
            if (arguments.isMember(name)) {
                if (!arguments[name].isString()) throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", std::string(name) + " must be a string");
                field = arguments[name].asString();
            }
        };
        text("origin", criteria.origin); text("destination", criteria.destination); text("status", criteria.status); text("airline", criteria.airline);
        if (arguments.isMember("terminal_id")) {
            if (!arguments["terminal_id"].isInt()) throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "terminal_id must be an integer");
            criteria.terminalId = arguments["terminal_id"].asInt();
        }
        return FlightService{}.searchFlights(criteria);
    });
}

Json::Value Tools::get_all_gates()
{
    return safely([] { return GateService{}.getAllGates(); });
}

Json::Value Tools::get_gate_by_id(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("gate_id") || !arguments["gate_id"].isInt())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "gate_id must be an integer");
        return GateService{}.getGateById(arguments["gate_id"].asInt());
    });
}

Json::Value Tools::get_gate_by_number(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("gate_number") || !arguments["gate_number"].isString())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "gate_number must be a string");
        return GateService{}.getGateByNumber(arguments["gate_number"].asString());
    });
}

// Returns all gates and their status.
Json::Value Tools::get_available_gates()
{
    // Only gates whose status is AVAILABLE, matching the tool name.
    return safely([] { return GateService{}.getAvailableGates(); });
}

Json::Value Tools::get_terminal_status(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("terminal_id") || !arguments["terminal_id"].isInt())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "terminal_id must be an integer");
        return terminalStatusToJson(TerminalService{}.getTerminalStatus(arguments["terminal_id"].asInt()));
    });
}

Json::Value Tools::get_flights_by_terminal(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("terminal_id") || !arguments["terminal_id"].isInt())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "terminal_id must be an integer");
        return TerminalService{}.getFlightsByTerminal(arguments["terminal_id"].asInt());
    });
}

// Returns all runways and their status.
Json::Value Tools::get_runway_status()
{
    return safely([] { return RunwayService{}.getStatus(); });
}

Json::Value Tools::get_runway_by_id(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("runway_id") || !arguments["runway_id"].isInt())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "runway_id must be an integer");
        return RunwayService{}.getById(std::to_string(arguments["runway_id"].asInt()));
    });
}

Json::Value Tools::get_runway_by_code(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("runway_code") || !arguments["runway_code"].isString())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "runway_code must be a string");
        return RunwayService{}.getByCode(arguments["runway_code"].asString());
    });
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

Json::Value Tools::update_flight_status(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments["flight_id"].isInt() || !arguments["status"].isString())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "flight_id and status are required");
        return FlightService{}.updateFlightStatus(std::to_string(arguments["flight_id"].asInt()), arguments["status"].asString());
    });
}

Json::Value Tools::assign_flight_to_gate(const Json::Value &arguments)
{
    return safely([&] {
        std::string flightId;
        if (arguments.isMember("flight_id") && arguments["flight_id"].isInt()) flightId = std::to_string(arguments["flight_id"].asInt());
        else if (arguments.isMember("flight_number") && arguments["flight_number"].isString())
            flightId = std::to_string(FlightService{}.getByNumber(arguments["flight_number"].asString())["id"].asInt());
        else throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "flight_id or flight_number is required");

        std::string gateId;
        if (arguments.isMember("gate_id") && arguments["gate_id"].isInt()) gateId = std::to_string(arguments["gate_id"].asInt());
        else if (arguments.isMember("gate_number") && arguments["gate_number"].isString())
            gateId = std::to_string(GateService{}.getByNumber(arguments["gate_number"].asString())["id"].asInt());
        else throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "gate_id or gate_number is required");
        return FlightService{}.assignFlightToGate(flightId, gateId);
    });
}

// Opens or closes a runway and reports the flights it affects.
Json::Value Tools::update_runway_status(const Json::Value &arguments)
{
    return safely([&] {
        if (!arguments.isMember("status") || !arguments["status"].isString())
            throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "status is required");
        const std::string status = arguments["status"].asString();
        if (arguments.isMember("runway_code") && arguments["runway_code"].isString())
            return RunwayService{}.updateStatusByCode(arguments["runway_code"].asString(), status);
        if (arguments.isMember("runway_id") && arguments["runway_id"].isInt())
            return RunwayService{}.updateStatus(std::to_string(arguments["runway_id"].asInt()), status);
        throw DomainError(DomainErrorKind::Validation, "invalid_tool_arguments", "runway_code or runway_id is required");
    });
}