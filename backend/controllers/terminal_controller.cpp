#include "terminal_controller.h"
#include "models/terminal.h"
#include "services/domain_error.h"
#include "services/terminal_service.h"
#include <charconv>
#include <iostream>

namespace
{
HttpResponsePtr success(const Json::Value &data)
{
    Json::Value body; body["status"] = "success"; body["data"] = data;
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); return response;
}

int positiveId(const std::string &raw)
{
    int id = 0;
    const auto [end, error] = std::from_chars(raw.data(), raw.data() + raw.size(), id);
    if (raw.empty() || error != std::errc{} || end != raw.data() + raw.size() || id <= 0)
        throw DomainError(DomainErrorKind::Validation, "invalid_terminal_id", "Terminal ID must be a positive integer");
    return id;
}

void domainFailure(const DomainError &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    Json::Value body; body["error"] = error.what(); body["code"] = error.code();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(error.kind() == DomainErrorKind::NotFound ? k404NotFound : k400BadRequest);
    callback(response);
}

void internalFailure(const std::exception &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    std::cerr << "Terminal request error: " << error.what() << std::endl;
    Json::Value body; body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k500InternalServerError); callback(response);
}
}

void TerminalController::getTerminals(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try {
        Json::Value terminals(Json::arrayValue);
        for (const auto &terminal : TerminalService{}.getAllTerminals()) terminals.append(terminalToJson(terminal));
        callback(success(terminals));
    } catch (const std::exception &error) { internalFailure(error, callback); }
}

void TerminalController::getTerminalById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try { callback(success(terminalToJson(TerminalService{}.getTerminalById(positiveId(id))))); }
    catch (const DomainError &error) { domainFailure(error, callback); }
    catch (const std::exception &error) { internalFailure(error, callback); }
}

void TerminalController::getTerminalStatus(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try { callback(success(terminalStatusToJson(TerminalService{}.getTerminalStatus(positiveId(id))))); }
    catch (const DomainError &error) { domainFailure(error, callback); }
    catch (const std::exception &error) { internalFailure(error, callback); }
}

void TerminalController::getTerminalFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try { callback(success(TerminalService{}.getFlightsByTerminal(positiveId(id)))); }
    catch (const DomainError &error) { domainFailure(error, callback); }
    catch (const std::exception &error) { internalFailure(error, callback); }
}

