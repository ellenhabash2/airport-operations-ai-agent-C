#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class IncidentController : public HttpController<IncidentController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(IncidentController::getIncidents, "/incidents", Get);
    ADD_METHOD_TO(IncidentController::createIncident, "/incidents", Post, "JwtAuthFilter");
    ADD_METHOD_TO(IncidentController::resolveIncident, "/incidents/{1}/resolve", Patch, "JwtAuthFilter");
    METHOD_LIST_END

    void getIncidents(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void createIncident(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void resolveIncident(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};
