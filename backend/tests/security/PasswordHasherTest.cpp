#include <gtest/gtest.h>
#include "security/PasswordHasher.h"

TEST(PasswordHasherTest, GeneratesNonPlaintextHash)
{
    const auto hash = PasswordHasher::hash("correct horse battery staple");
    EXPECT_FALSE(hash.empty());
    EXPECT_NE(hash, "correct horse battery staple");
}

TEST(PasswordHasherTest, VerifiesCorrectPassword)
{
    const auto hash = PasswordHasher::hash("airport-secret");
    EXPECT_TRUE(PasswordHasher::verify("airport-secret", hash));
}

TEST(PasswordHasherTest, RejectsIncorrectPassword)
{
    const auto hash = PasswordHasher::hash("airport-secret");
    EXPECT_FALSE(PasswordHasher::verify("wrong-secret", hash));
}

TEST(PasswordHasherTest, UsesUniqueSalt)
{
    const auto first = PasswordHasher::hash("same-password");
    const auto second = PasswordHasher::hash("same-password");
    EXPECT_NE(first, second);
    EXPECT_TRUE(PasswordHasher::verify("same-password", first));
    EXPECT_TRUE(PasswordHasher::verify("same-password", second));
}
