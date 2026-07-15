#pragma once
#include <string>

// Minimal client for the Google Gemini generateContent REST API.
// Reads the API key from the GEMINI_API_KEY environment variable.
class GeminiClient
{
public:
    GeminiClient();

    // Sends a text question to Gemini and returns the text answer.
    // On error, returns a string starting with "ERROR:".
    std::string ask(const std::string &question);

private:
    std::string api_key_;
};
