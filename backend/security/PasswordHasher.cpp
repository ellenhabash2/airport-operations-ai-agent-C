#include "PasswordHasher.h"
#include <crypt.h>
#include <array>
#include <random>
#include <stdexcept>

std::string PasswordHasher::hash(const std::string &password)
{
    std::array<char, CRYPT_GENSALT_OUTPUT_SIZE> salt{};
    std::array<unsigned char, 16> entropy{};
    std::random_device random;
    for (auto &byte : entropy) byte = static_cast<unsigned char>(random());
    if (!crypt_gensalt_rn("$2b$", 10, reinterpret_cast<const char *>(entropy.data()),
                          entropy.size(), salt.data(), salt.size()))
        throw std::runtime_error("Could not generate bcrypt salt");
    crypt_data data{};
    const char *hashed = crypt_r(password.c_str(), salt.data(), &data);
    if (!hashed) throw std::runtime_error("Could not hash password");
    return hashed;
}

bool PasswordHasher::verify(const std::string &password, const std::string &hash)
{
    crypt_data data{};
    const char *candidate = crypt_r(password.c_str(), hash.c_str(), &data);
    return candidate && hash == candidate;
}
