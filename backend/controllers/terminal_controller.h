#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class TerminalController : public HttpController<TerminalController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TerminalController::getTerminals, "/terminals", Get);
    ADD_METHOD_TO(TerminalController::getTerminalStatus, "/terminals/{1}/status", Get);
    ADD_METHOD_TO(TerminalController::getTerminalFlights, "/terminals/{1}/flights", Get);
    ADD_METHOD_TO(TerminalController::getTerminalById, "/terminals/{1}", Get);
    METHOD_LIST_END

    void getTerminals(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&);
    void getTerminalById(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
    void getTerminalStatus(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
    void getTerminalFlights(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&, std::string id);
};

