#include "flight_controller.h"
#include "services/domain_error.h"
#include "services/flight_service.h"
#include <charconv>
#include <iostream>

namespace
{
HttpResponsePtr success(const Json::Value &data, bool includeCount = false)
{
    Json::Value body; body["status"] = "success"; body["data"] = data;
    if (includeCount) body["count"] = static_cast<Json::UInt64>(data.size());
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); return response;
}

HttpStatusCode domainStatus(const DomainError &error)
{
    if (error.kind() == DomainErrorKind::NotFound) return k404NotFound;
    if (error.kind() == DomainErrorKind::Conflict) return k409Conflict;
    return k400BadRequest;
}

void domainFailure(const DomainError &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    Json::Value body; body["error"] = error.what(); body["code"] = error.code();
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(domainStatus(error)); callback(response);
}

void internalFailure(const std::exception &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    std::cerr << "Flight request error: " << error.what() << std::endl;
    Json::Value body; body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k500InternalServerError); callback(response);
}

std::optional<int> queryInteger(const HttpRequestPtr &request, const std::string &name)
{
    auto raw = request->getParameter(name);
    if (raw.empty()) return std::nullopt;
    int value = 0; auto [end, error] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (error != std::errc{} || end != raw.data() + raw.size())
        throw DomainError(DomainErrorKind::Validation, "invalid_terminal_id", "Terminal ID must be a positive integer");
    return value;
}
}

void FlightController::getFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{ try { callback(success(FlightService{}.getAll())); } catch (const std::exception &e) { internalFailure(e, callback); } }

void FlightController::getDelayedFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{ try { callback(success(FlightService{}.getDelayed(), true)); } catch (const std::exception &e) { internalFailure(e, callback); } }

void FlightController::getFlightById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try { callback(success(FlightService{}.getById(id))); }
    catch (const DomainError &e) { domainFailure(e, callback); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}

void FlightController::getFlightByNumber(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string number)
{
    try { callback(success(FlightService{}.getByNumber(number))); }
    catch (const DomainError &e) { domainFailure(e, callback); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}

void FlightController::searchFlights(const HttpRequestPtr &request, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try {
        FlightSearchCriteria criteria;
        auto set = [&](const char *name, std::optional<std::string> &field) { auto value = request->getParameter(name); if (!value.empty()) field = value; };
        set("origin", criteria.origin); set("destination", criteria.destination); set("status", criteria.status); set("airline", criteria.airline);
        criteria.terminalId = queryInteger(request, "terminal_id");
        callback(success(FlightService{}.searchFlights(criteria), true));
    } catch (const DomainError &e) { domainFailure(e, callback); }
      catch (const std::exception &e) { internalFailure(e, callback); }
}

void FlightController::updateFlightStatus(const HttpRequestPtr &request, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try {
        auto body = request->getJsonObject();
        if (!body || !body->isMember("status") || !(*body)["status"].isString())
            throw DomainError(DomainErrorKind::Validation, "invalid_status_body", "A string status field is required");
        callback(success(FlightService{}.updateFlightStatus(id, (*body)["status"].asString())));
    } catch (const DomainError &e) { domainFailure(e, callback); }
      catch (const std::exception &e) { internalFailure(e, callback); }
}

void FlightController::assignFlightGate(const HttpRequestPtr &request, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try {
        auto body = request->getJsonObject();
        if (!body || !body->isMember("gate_id") || !(*body)["gate_id"].isInt())
            throw DomainError(DomainErrorKind::Validation, "invalid_gate_body", "An integer gate_id field is required");
        callback(success(FlightService{}.assignFlightToGate(id, std::to_string((*body)["gate_id"].asInt()))));
    } catch (const DomainError &e) { domainFailure(e, callback); }
      catch (const std::exception &e) { internalFailure(e, callback); }
}
