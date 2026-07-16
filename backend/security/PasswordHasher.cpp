#include "PasswordHasher.h"
#include "bcrypt.h"

std::string PasswordHasher::hash(const std::string &password)
{
    return bcrypt::generateHash(password);
}

bool PasswordHasher::verify(const std::string &password, const std::string &hash)
{
    return bcrypt::validatePassword(password, hash);
}
