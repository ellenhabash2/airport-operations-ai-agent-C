#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

// Temporary controller to test agent tools directly (before Gemini integration).
class ToolsTestController : public HttpController<ToolsTestController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ToolsTestController::testTools, "/tools/test", Get);
    METHOD_LIST_END

    void testTools(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
