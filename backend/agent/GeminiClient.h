#pragma once
#include <string>
#include <json/json.h>

// AI client backed by Groq (OpenAI-compatible API).
class GeminiClient
{
public:
    GeminiClient();

    // Simple prompt -> text answer (used for quick tests).
    std::string ask(const std::string &question);

    // Full chat completion with tool support.
    // 'messages' is the OpenAI-format conversation array.
    // 'tools' is the OpenAI-format tools array (may be empty).
    // Returns the raw parsed JSON response from the provider.
    Json::Value chatWithTools(const Json::Value &messages, const Json::Value &tools);

private:
    std::string api_key_;
};
