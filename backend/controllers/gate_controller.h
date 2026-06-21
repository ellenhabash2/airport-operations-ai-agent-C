#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class GateController : public HttpController<GateController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GateController::getGates, "/gates", Get);
    METHOD_LIST_END

    void getGates(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
