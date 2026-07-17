#include "auth_controller.h"
#include <drogon/HttpAppFramework.h>
#include <iostream>
#include <json/json.h>
#include <algorithm>
#include <cctype>
#include <string>
#include "repositories/user_repository.h"
#include "security/PasswordHasher.h"
#include "security/JwtService.h"

namespace
{
std::string trim(const std::string &s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Permissive sanity check, not a full RFC validator: needs a non-empty local
// part, an '@', a '.' after it, and no spaces.
bool looksLikeEmail(const std::string &e)
{
    const auto at = e.find('@');
    if (at == std::string::npos || at == 0) return false;
    const auto dot = e.find('.', at + 1);
    if (dot == std::string::npos || dot + 1 >= e.size()) return false;
    return e.find_first_of(" \t\r\n") == std::string::npos;
}
}

void AuthController::registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json || !json->isMember("username") || !json->isMember("email") || !json->isMember("password"))
        {
            Json::Value error_response;
            error_response["error"] = "Missing required fields: username, email, password";
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        std::string username = trim((*json)["username"].asString());
        std::string email = toLower(trim((*json)["email"].asString()));
        std::string password = (*json)["password"].asString();

        if (username.length() < 2 || !looksLikeEmail(email) || password.length() < 6)
        {
            Json::Value error_response;
            error_response["error"] = "Invalid input: username must be at least 2 characters, "
                                      "a valid email is required, and password must be at least 6 characters";
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        // Hash the password before storing
        std::string hash = PasswordHasher::hash(password);

        auto created = UserRepository::createUser(username, email, hash);

        // Duplicate email/username
        if (created.isMember("error"))
        {
            Json::Value error_response;
            error_response["error"] = created["error"];
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k409Conflict);
            callback(http_response);
            return;
        }

        Json::Value response;
        response["status"] = "success";
        response["message"] = "User registered";
        response["data"] = created;

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k201Created);
        callback(http_response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";
        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}

void AuthController::loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json || !json->isMember("email") || !json->isMember("password"))
        {
            Json::Value error_response;
            error_response["error"] = "Missing required fields: email, password";
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k400BadRequest);
            callback(http_response);
            return;
        }

        std::string email = toLower(trim((*json)["email"].asString()));
        std::string password = (*json)["password"].asString();

        auto user = UserRepository::findByEmail(email);

        // User not found
        if (user.isNull() || !user.isMember("password_hash"))
        {
            Json::Value error_response;
            error_response["error"] = "Invalid email or password";
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k401Unauthorized);
            callback(http_response);
            return;
        }

        // Verify password against stored hash
        std::string storedHash = user["password_hash"].asString();
        if (!PasswordHasher::verify(password, storedHash))
        {
            Json::Value error_response;
            error_response["error"] = "Invalid email or password";
            auto http_response = HttpResponse::newHttpJsonResponse(error_response);
            http_response->setStatusCode(k401Unauthorized);
            callback(http_response);
            return;
        }

        // Issue a JWT token
        std::string userId = user["id"].asString();
        std::string token = JwtService::generateToken(userId, email);

        Json::Value response;
        response["status"] = "success";
        response["message"] = "Login successful";
        response["token"] = token;
        response["user"]["id"] = user["id"];
        response["user"]["username"] = user["username"];
        response["user"]["email"] = user["email"];
        response["user"]["role"] = user["role"];

        auto http_response = HttpResponse::newHttpJsonResponse(response);
        http_response->setStatusCode(k200OK);
        callback(http_response);
    }
    catch (const std::exception &e)
    {
        Json::Value error_response;
        std::cerr << "Request error: " << e.what() << std::endl;
        error_response["error"] = "Internal server error";
        auto http_response = HttpResponse::newHttpJsonResponse(error_response);
        http_response->setStatusCode(k500InternalServerError);
        callback(http_response);
    }
}
