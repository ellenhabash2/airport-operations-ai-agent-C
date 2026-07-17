#include "incident_controller.h"
#include <drogon/HttpAppFramework.h>
#include <iostream>
#include <json/json.h>
#include "repositories/incident_repository.h"
#include <array>
#include <algorithm>
#include <cctype>

namespace
{
bool isValidSeverity(const std::string &severity)
{
    static constexpr std::array<const char *, 4> allowed = {"LOW", "MEDIUM", "HIGH", "CRITICAL"};
    return std::find(allowed.begin(), allowed.end(), severity) != allowed.end();
}
}

void IncidentController::getIncidents(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto incidents = IncidentRepository::getAllIncidents();

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

        if (title.empty() || description.empty() || !isValidSeverity(severity))
        {
            Json::Value error_response;
            error_response["error"] = "Invalid incident payload";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        auto incident = IncidentRepository::createIncident(title, description, severity, location);

        Json::Value response;
        response["status"] = "success";
        response["data"] = incident;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k201Created);
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

void IncidentController::resolveIncident(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id)
{
    try
    {
        // Validate the id is a positive integer
        if (id.empty() || !std::all_of(id.begin(), id.end(), [](unsigned char ch) { return std::isdigit(ch); }))
        {
            Json::Value error_response;
            error_response["error"] = "Incident ID must be a positive integer";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        auto result = IncidentRepository::resolveIncident(id);

        // Incident does not exist
        if (!result["found"].asBool())
        {
            Json::Value error_response;
            error_response["error"] = "Incident not found";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k404NotFound);
            callback(http_response);
            return;
        }

        // Business rule: already resolved
        if (result["already_resolved"].asBool())
        {
            Json::Value error_response;
            error_response["error"] = "Incident is already resolved";

            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k409Conflict);
            callback(http_response);
            return;
        }

        // Success
        Json::Value response;
        response["status"] = "success";
        response["message"] = "Incident resolved";
        response["data"] = result["incident"];

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
