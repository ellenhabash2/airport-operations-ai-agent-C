#include <gtest/gtest.h>
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <cstdlib>
#include "security/JwtService.h"

namespace {
class JwtServiceTest : public testing::Test {
protected:
    void SetUp() override { setenv("JWT_SECRET", "unit-test-secret-that-is-not-production", 1); }
    void TearDown() override { unsetenv("JWT_SECRET"); }
};
}

TEST_F(JwtServiceTest, GeneratesAndVerifiesToken)
{
    const auto token = JwtService::generateToken("42", "operator@example.com");
    EXPECT_FALSE(token.empty());
    EXPECT_TRUE(JwtService::verifyToken(token));
}

TEST_F(JwtServiceTest, ExtractsUserId)
{
    EXPECT_EQ(JwtService::getUserId(
        JwtService::generateToken("42", "operator@example.com")), "42");
}

TEST_F(JwtServiceTest, RejectsTamperedToken)
{
    auto token = JwtService::generateToken("42", "operator@example.com");
    const auto signatureStart = token.rfind('.') + 1;
    ASSERT_LT(signatureStart, token.size());
    token[signatureStart] = token[signatureStart] == 'a' ? 'b' : 'a';
    EXPECT_FALSE(JwtService::verifyToken(token));
    EXPECT_TRUE(JwtService::getUserId(token).empty());
}

TEST_F(JwtServiceTest, RejectsInvalidSignature)
{
    const auto token = JwtService::generateToken("42", "operator@example.com");
    setenv("JWT_SECRET", "different-unit-test-secret", 1);
    EXPECT_FALSE(JwtService::verifyToken(token));
}

TEST_F(JwtServiceTest, RejectsExpiredToken)
{
    const auto expired = jwt::create()
        .set_issuer("aeromind")
        .set_payload_claim("user_id", jwt::claim(std::string("42")))
        .set_expires_at(std::chrono::system_clock::now() - std::chrono::seconds(1))
        .sign(jwt::algorithm::hs256{"unit-test-secret-that-is-not-production"});
    EXPECT_FALSE(JwtService::verifyToken(expired));
}

TEST_F(JwtServiceTest, RequiresConfiguredSecret)
{
    unsetenv("JWT_SECRET");
    EXPECT_THROW(JwtService::generateToken("42", "operator@example.com"), std::runtime_error);
}
