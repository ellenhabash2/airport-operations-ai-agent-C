#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

// Temporary controller to test JWT generation and verification.
class JwtTestController : public HttpController<JwtTestController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(JwtTestController::testJwt, "/jwt/test", Get);
    METHOD_LIST_END

    void testJwt(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
