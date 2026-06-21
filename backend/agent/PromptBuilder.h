#pragma once
#include <string>

class PromptBuilder
{
public:
    virtual ~PromptBuilder() = default;

    // TODO(ai-phase): Compose system and operational context prompts for Gemini.
    virtual std::string buildOperationsPrompt(const std::string &user_query) const = 0;
};
