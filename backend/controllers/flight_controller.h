#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class FlightController : public HttpController<FlightController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(FlightController::getFlights, "/flights", Get);
    ADD_METHOD_TO(FlightController::getDelayedFlights, "/flights/delayed", Get);
    ADD_METHOD_TO(FlightController::searchFlights, "/flights/search", Get);
    ADD_METHOD_TO(FlightController::getFlightByNumber, "/flights/number/{1}", Get);
    ADD_METHOD_TO(FlightController::updateFlightStatus, "/flights/{1}/status", Patch, "JwtAuthFilter");
    ADD_METHOD_TO(FlightController::assignFlightGate, "/flights/{1}/gate", Patch, "JwtAuthFilter");
    ADD_METHOD_TO(FlightController::getFlightById, "/flights/{1}", Get);
    METHOD_LIST_END

    void getFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&);
    void getDelayedFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&);
    void searchFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&);
    void getFlightByNumber(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string flightNumber);
    void getFlightById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
    void updateFlightStatus(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
    void assignFlightGate(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
};
