#include "gate_controller.h"
#include <iostream>
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "services/gate_service.h"
#include "services/domain_error.h"
#include <charconv>

namespace
{
HttpResponsePtr success(const Json::Value &data)
{
    Json::Value body; body["status"] = "success"; body["data"] = data;
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); return response;
}

void failure(const DomainError &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    Json::Value body; body["error"] = error.what(); body["code"] = error.code();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(error.kind() == DomainErrorKind::NotFound ? k404NotFound : k400BadRequest);
    callback(response);
}

void internalFailure(const std::exception &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    std::cerr << "Gate request error: " << error.what() << std::endl;
    Json::Value body; body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k500InternalServerError); callback(response);
}

int positiveId(const std::string &raw)
{
    int id = 0;
    const auto [end, error] = std::from_chars(raw.data(), raw.data() + raw.size(), id);
    if (raw.empty() || error != std::errc{} || end != raw.data() + raw.size() || id <= 0)
        throw DomainError(DomainErrorKind::Validation, "invalid_gate_id", "Gate ID must be a positive integer");
    return id;
}
}

void GateController::getGates(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        callback(success(GateService{}.getAllGates()));
    }
    catch (const std::exception &e)
    {
        internalFailure(e, callback);
    }
}

void GateController::getAvailableGates(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try { callback(success(GateService{}.getAvailableGates())); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}

void GateController::getGateById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try { callback(success(GateService{}.getGateById(positiveId(id)))); }
    catch (const DomainError &e) { failure(e, callback); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}

void GateController::getGateByNumber(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string gateNumber)
{
    try { callback(success(GateService{}.getGateByNumber(gateNumber))); }
    catch (const DomainError &e) { failure(e, callback); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}
