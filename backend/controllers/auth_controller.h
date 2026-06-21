#pragma once
#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

class AuthController : public HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/auth/register", Post);
    ADD_METHOD_TO(AuthController::loginUser, "/auth/login", Post);
    METHOD_LIST_END

    void registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
