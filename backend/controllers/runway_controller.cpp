#include "runway_controller.h"
#include "services/domain_error.h"
#include "services/runway_service.h"
#include <iostream>

namespace
{
HttpResponsePtr success(const Json::Value &data)
{
    Json::Value body;
    body["status"] = "success";
    body["data"] = data;
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    return response;
}

HttpStatusCode domainStatus(const DomainError &error)
{
    if (error.kind() == DomainErrorKind::NotFound) return k404NotFound;
    if (error.kind() == DomainErrorKind::Conflict) return k409Conflict;
    return k400BadRequest;
}

void domainFailure(const DomainError &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    Json::Value body;
    body["error"] = error.what();
    body["code"] = error.code();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(domainStatus(error));
    callback(response);
}

void internalFailure(const std::exception &error, const std::function<void(const HttpResponsePtr &)> &callback)
{
    std::cerr << "Runway request error: " << error.what() << std::endl;
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
}
}

void RunwayController::getRunways(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try {
        callback(success(RunwayService{}.getStatus()));
    } catch (const std::exception &e) {
        internalFailure(e, callback);
    }
}

void RunwayController::getRunwayById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try { callback(success(RunwayService{}.getById(id))); }
    catch (const DomainError &e) { domainFailure(e, callback); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}

void RunwayController::getRunwayByCode(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback, std::string code)
{
    try { callback(success(RunwayService{}.getByCode(code))); }
    catch (const DomainError &e) { domainFailure(e, callback); }
    catch (const std::exception &e) { internalFailure(e, callback); }
}

void RunwayController::updateStatus(const HttpRequestPtr &request, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try {
        auto body = request->getJsonObject();
        if (!body || !body->isMember("status") || !(*body)["status"].isString())
            throw DomainError(DomainErrorKind::Validation, "invalid_status_body", "A string status field is required");
        callback(success(RunwayService{}.updateStatus(id, (*body)["status"].asString())));
    } catch (const DomainError &e) {
        domainFailure(e, callback);
    } catch (const std::exception &e) {
        internalFailure(e, callback);
    }
}
