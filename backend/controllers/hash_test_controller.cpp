#include "hash_test_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "security/PasswordHasher.h"

void HashTestController::testHash(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        std::string password = "MySecret123";
        std::string hash = PasswordHasher::hash(password);

        bool correctMatches = PasswordHasher::verify(password, hash);
        bool wrongMatches = PasswordHasher::verify("WrongPassword", hash);

        Json::Value response;
        response["status"] = "success";
        response["password"] = password;
        response["hash"] = hash;
        response["correct_password_verifies"] = correctMatches;
        response["wrong_password_verifies"] = wrongMatches;

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
