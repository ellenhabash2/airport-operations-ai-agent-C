#include "JwtAuthFilter.h"
#include "security/JwtService.h"
#include <json/json.h>

namespace
{
// Small helper: build a 401 JSON response with a given message.
HttpResponsePtr unauthorized(const std::string &message)
{
    Json::Value err;
    err["error"] = message;
    auto resp = HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(k401Unauthorized);
    return resp;
}
}

void JwtAuthFilter::doFilter(const HttpRequestPtr &req,
                             FilterCallback &&fcb,
                             FilterChainCallback &&fccb)
{
    // Expected header format: "Authorization: Bearer <token>"
    const std::string authHeader = req->getHeader("Authorization");
    const std::string prefix = "Bearer ";

    // rfind(prefix, 0) == 0 means the string starts with prefix.
    if (authHeader.rfind(prefix, 0) != 0)
    {
        fcb(unauthorized("Missing or malformed Authorization header. "
                         "Expected format: Bearer <token>"));
        return;
    }

    const std::string token = authHeader.substr(prefix.size());

    if (token.empty() || !JwtService::verifyToken(token))
    {
        fcb(unauthorized("Invalid or expired token"));
        return;
    }

    // Token is valid, allow the request through to the controller.
    fccb();
}