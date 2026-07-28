#include "LLMConfig.h"
#include <cstdlib>

namespace {
std::string readEnvironment(const char *name, const std::string &fallback = {})
{
    const char *value = std::getenv(name);
    return value && *value ? value : fallback;
}
}

LLMConfig LLMConfig::fromEnvironment()
{
    LLMConfig config;
    config.provider = readEnvironment("AI_PROVIDER", "gemini");
    config.apiKey = readEnvironment("GEMINI_API_KEY");
    config.model = readEnvironment("GEMINI_MODEL", "gemini-2.5-flash");
    config.baseUrl = readEnvironment(
        "GEMINI_BASE_URL",
        "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
    return config;
}

std::string LLMConfig::validationError() const
{
    if (provider != "gemini") return "AI_PROVIDER must be set to gemini";
    if (apiKey.empty()) return "GEMINI_API_KEY is not configured";
    if (model.empty()) return "GEMINI_MODEL is not configured";
    if (baseUrl.rfind("https://", 0) != 0)
        return "GEMINI_BASE_URL must use HTTPS";
    return {};
}
