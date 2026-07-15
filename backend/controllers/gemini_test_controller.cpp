#include "gemini_test_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "agent/GeminiClient.h"

void GeminiTestController::testGemini(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        GeminiClient client;
        std::string answer = client.ask("Say hello and confirm you are working, in one short sentence.");

        Json::Value response;
        response["status"] = "success";
        response["gemini_reply"] = answer;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k200OK);
        callback(http_response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        error_response["error"] = e.what();
        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}
