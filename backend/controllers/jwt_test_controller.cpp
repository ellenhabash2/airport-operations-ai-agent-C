#include "jwt_test_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "security/JwtService.h"

void JwtTestController::testJwt(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        // Generate a token for a sample user
        std::string token = JwtService::generateToken("42", "test@aeromind.com");

        // Verify it and extract the user id
        bool valid = JwtService::verifyToken(token);
        std::string userId = JwtService::getUserId(token);

        // Verify a tampered token should fail
        bool tamperedValid = JwtService::verifyToken(token + "tampered");

        Json::Value response;
        response["status"] = "success";
        response["token"] = token;
        response["valid_token_verifies"] = valid;
        response["extracted_user_id"] = userId;
        response["tampered_token_verifies"] = tamperedValid;

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
