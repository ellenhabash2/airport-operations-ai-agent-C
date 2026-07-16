#include "JwtService.h"
#include <jwt-cpp/jwt.h>
#include <cstdlib>
#include <chrono>

std::string JwtService::getSecret()
{
    const char *s = std::getenv("JWT_SECRET");
    return s ? s : "default_dev_secret";
}

std::string JwtService::generateToken(const std::string &userId, const std::string &email)
{
    auto token = jwt::create()
        .set_issuer("aeromind")
        .set_type("JWS")
        .set_payload_claim("user_id", jwt::claim(userId))
        .set_payload_claim("email", jwt::claim(email))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
        .sign(jwt::algorithm::hs256{getSecret()});

    return token;
}

bool JwtService::verifyToken(const std::string &token)
{
    try
    {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{getSecret()})
            .with_issuer("aeromind");
        verifier.verify(decoded);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

std::string JwtService::getUserId(const std::string &token)
{
    try
    {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{getSecret()})
            .with_issuer("aeromind");
        verifier.verify(decoded);
        return decoded.get_payload_claim("user_id").as_string();
    }
    catch (const std::exception &)
    {
        return "";
    }
}
