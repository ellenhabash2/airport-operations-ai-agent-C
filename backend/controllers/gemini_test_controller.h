#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

// Temporary controller to test the Gemini connection.
class GeminiTestController : public HttpController<GeminiTestController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(GeminiTestController::testGemini, "/gemini/test", Get);
    METHOD_LIST_END

    void testGemini(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
