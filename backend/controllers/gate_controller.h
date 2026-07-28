#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class GateController : public HttpController<GateController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GateController::getGates, "/gates", Get);
    ADD_METHOD_TO(GateController::getAvailableGates, "/gates/available", Get);
    ADD_METHOD_TO(GateController::getGateByNumber, "/gates/number/{1}", Get);
    ADD_METHOD_TO(GateController::getGateById, "/gates/{1}", Get);
    METHOD_LIST_END

    void getGates(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getAvailableGates(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&);
    void getGateById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
    void getGateByNumber(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string gateNumber);
};
