#pragma once
#include <string>

// Wraps bcrypt password hashing for the auth module.
class PasswordHasher
{
public:
    // Hashes a plaintext password and returns the bcrypt hash string.
    static std::string hash(const std::string &password);

    // Returns true if the plaintext password matches the stored hash.
    static bool verify(const std::string &password, const std::string &hash);
};
