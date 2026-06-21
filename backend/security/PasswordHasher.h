#pragma once
#include <string>

class PasswordHasher
{
public:
    virtual ~PasswordHasher() = default;

    // TODO(auth-phase): Hash passwords with a production password hashing algorithm.
    virtual std::string hashPassword(const std::string &password) const = 0;

    // TODO(auth-phase): Verify submitted credentials against stored password hashes.
    virtual bool verifyPassword(const std::string &password, const std::string &hash) const = 0;
};
