#pragma once
#include <string>

struct LLMConfig
{
    std::string provider{"gemini"};
    std::string apiKey;
    std::string model{"gemini-2.5-flash"};
    std::string baseUrl{
        "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions"};
    int requestTimeoutSeconds{30};
    int maxAttempts{4};

    static LLMConfig fromEnvironment();
    std::string validationError() const;
};
