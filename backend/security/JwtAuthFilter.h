#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

// Rejects any request that does not carry a valid, unexpired JWT in the
// Authorization header ("Bearer <token>"). Attach it to a route by passing
// "JwtAuthFilter" as the last argument of ADD_METHOD_TO.
class JwtAuthFilter : public HttpFilter<JwtAuthFilter>
{
public:
    JwtAuthFilter() = default;
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};