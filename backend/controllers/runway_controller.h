#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class RunwayController : public HttpController<RunwayController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RunwayController::getRunways, "/runways", Get);
    METHOD_LIST_END

    void getRunways(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
