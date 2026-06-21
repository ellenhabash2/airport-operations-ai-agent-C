#pragma once
#include <string>

class GeminiClient
{
public:
    virtual ~GeminiClient() = default;

    // TODO(ai-phase): Call Gemini with configured credentials in a future phase.
    virtual std::string generate(const std::string &prompt) = 0;
};
