#include <gtest/gtest.h>
#include <cstdlib>
#include "agent/LLMConfig.h"

namespace {
class LLMConfigTest : public testing::Test {
protected:
    void TearDown() override {
        unsetenv("AI_PROVIDER"); unsetenv("GEMINI_API_KEY");
        unsetenv("GEMINI_MODEL"); unsetenv("GEMINI_BASE_URL");
    }
};
}

TEST_F(LLMConfigTest, UsesGeminiDefaults)
{
    unsetenv("AI_PROVIDER"); unsetenv("GEMINI_API_KEY");
    unsetenv("GEMINI_MODEL"); unsetenv("GEMINI_BASE_URL");
    const auto config = LLMConfig::fromEnvironment();
    EXPECT_EQ(config.provider, "gemini");
    EXPECT_EQ(config.model, "gemini-2.5-flash");
    EXPECT_EQ(config.baseUrl,
        "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
    EXPECT_FALSE(config.validationError().empty());
}

TEST_F(LLMConfigTest, ReadsConfiguredGeminiValues)
{
    setenv("AI_PROVIDER", "gemini", 1);
    setenv("GEMINI_API_KEY", "test-key", 1);
    setenv("GEMINI_MODEL", "configured-model", 1);
    setenv("GEMINI_BASE_URL", "https://example.test/chat", 1);
    const auto config = LLMConfig::fromEnvironment();
    EXPECT_EQ(config.apiKey, "test-key");
    EXPECT_EQ(config.model, "configured-model");
    EXPECT_EQ(config.baseUrl, "https://example.test/chat");
    EXPECT_TRUE(config.validationError().empty());
}

TEST_F(LLMConfigTest, RejectsNonHttpsEndpoint)
{
    LLMConfig config;
    config.apiKey = "test-key";
    config.baseUrl = "http://example.test/chat";
    EXPECT_EQ(config.validationError(), "GEMINI_BASE_URL must use HTTPS");
}
