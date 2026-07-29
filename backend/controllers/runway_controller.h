#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class RunwayController : public HttpController<RunwayController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RunwayController::getRunways, "/runways", Get);
    ADD_METHOD_TO(RunwayController::getRunwayByCode, "/runways/code/{1}", Get);
    ADD_METHOD_TO(RunwayController::getRunwayById, "/runways/{1}", Get);
    ADD_METHOD_TO(RunwayController::updateStatus, "/runways/{1}/status", Patch, "JwtAuthFilter");
    METHOD_LIST_END

    void getRunways(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getRunwayById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void getRunwayByCode(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string code);
    void updateStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};
