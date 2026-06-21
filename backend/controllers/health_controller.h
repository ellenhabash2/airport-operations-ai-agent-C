#pragma once
#include <drogon/HttpController.h>
#include <string>
#include <memory>

using namespace drogon;

class HealthController : public HttpController<HealthController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::health, "/health", Get);
    METHOD_LIST_END

    void health(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
