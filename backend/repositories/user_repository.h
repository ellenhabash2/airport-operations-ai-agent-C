#pragma once
#include <json/json.h>
#include <string>

class UserRepository
{
public:
    // Creates a user. Returns the created user (without password), or a
    // JSON object with "error" if the email/username already exists.
    static Json::Value createUser(const std::string &username, const std::string &email,
                                  const std::string &password_hash);

    // Finds a user by email. Returns the row including password_hash,
    // or null if not found.
    static Json::Value findByEmail(const std::string &email);
};
