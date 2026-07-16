#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

// Temporary controller to test password hashing.
class HashTestController : public HttpController<HashTestController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HashTestController::testHash, "/hash/test", Get);
    METHOD_LIST_END

    void testHash(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
