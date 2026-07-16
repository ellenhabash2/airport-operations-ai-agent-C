#pragma once
#include <string>

// Creates and verifies JWT tokens for user sessions.
class JwtService
{
public:
    // Generates a signed JWT for the given user. Token expires in 24 hours.
    static std::string generateToken(const std::string &userId, const std::string &email);

    // Verifies a token. Returns true if valid and not expired.
    static bool verifyToken(const std::string &token);

    // Extracts the user id from a valid token, or "" if invalid.
    static std::string getUserId(const std::string &token);

private:
    static std::string getSecret();
};
