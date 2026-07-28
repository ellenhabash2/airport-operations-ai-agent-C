#include "incident_controller.h"
#include <drogon/HttpAppFramework.h>
#include <iostream>
#include <json/json.h>
#include "services/domain_error.h"
#include "services/incident_service.h"

void IncidentController::getIncidents(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto incidents = IncidentService{}.getAll();

        Json::Value response;
        response["status"] = "success";
        response["data"] = incidents;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k200OK);
        callback(http_response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";

        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}

void IncidentController::createIncident(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json || !json->isMember("title") || !json->isMember("description") || !json->isMember("severity"))
        {
            Json::Value error_response;
            error_response["error"] = "Missing required fields: title, description, severity";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        std::string title = (*json)["title"].asString();
        std::string description = (*json)["description"].asString();
        std::string severity = (*json)["severity"].asString();
        std::string location = (*json).isMember("location") ? (*json)["location"].asString() : "";

        auto incident = IncidentService{}.create(title, description, severity, location);

        Json::Value response;
        response["status"] = "success";
        response["data"] = incident;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k201Created);
        callback(http_response);
    }
    catch (const DomainError &e)
    {
        Json::Value error; error["error"] = e.what();
        auto response = HttpResponse::newHttpJsonResponse(error); response->setStatusCode(k400BadRequest); callback(response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";

        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}

void IncidentController::resolveIncident(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try
    {
        auto incident = IncidentService{}.resolve(id);
        Json::Value response;
        response["status"] = "success";
        response["message"] = "Incident resolved";
        response["data"] = incident;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k200OK);
        callback(http_response);
    }
    catch (const DomainError &e)
    {
        Json::Value error; error["error"] = e.what();
        auto response = HttpResponse::newHttpJsonResponse(error);
        HttpStatusCode status = k400BadRequest;
        if (e.kind() == DomainErrorKind::NotFound) status = k404NotFound;
        else if (e.kind() == DomainErrorKind::Conflict) status = k409Conflict;
        response->setStatusCode(status); callback(response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";

        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}
