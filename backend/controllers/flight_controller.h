#pragma once
#include <drogon/HttpController.h>
#include <string>
#include <memory>

using namespace drogon;

class FlightController : public HttpController<FlightController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(FlightController::getFlights, "/flights", Get);
    ADD_METHOD_TO(FlightController::getDelayedFlights, "/flights/delayed", Get);
    ADD_METHOD_TO(FlightController::getFlightById, "/flights/{1}", Get);
    METHOD_LIST_END

    void getFlights(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getDelayedFlights(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getFlightById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};
