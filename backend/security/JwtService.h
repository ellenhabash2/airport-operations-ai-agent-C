#pragma once
#include <string>

class JwtService
{
public:
    virtual ~JwtService() = default;

    // TODO(auth-phase): Generate signed JWTs after authentication is implemented.
    virtual std::string issueToken(int user_id, const std::string &role) const = 0;

    // TODO(auth-phase): Verify signed JWTs for protected API routes.
    virtual bool verifyToken(const std::string &token) const = 0;
};
