#pragma once
#include "LLMConfig.h"
#include <functional>
#include <json/json.h>
#include <map>
#include <memory>
#include <string>

struct ChatHttpResponse
{
    bool networkOk{false};
    bool timedOut{false};
    int statusCode{0};
    std::string body;
};

class IChatHttpTransport
{
public:
    virtual ~IChatHttpTransport() = default;
    virtual ChatHttpResponse post(
        const std::string &url,
        const std::map<std::string, std::string> &headers,
        const std::string &body,
        int timeoutSeconds) = 0;
};

// Provider-neutral OpenAI-compatible chat client. The active provider is
// configured through LLMConfig; AeroMind currently supports Gemini.
class LLMClient
{
public:
    using RetryDelay = std::function<void(int attempt)>;

    LLMClient();
    explicit LLMClient(LLMConfig config,
                       std::shared_ptr<IChatHttpTransport> transport = nullptr,
                       RetryDelay retryDelay = nullptr);

    std::string ask(const std::string &question);
    Json::Value chatWithTools(const Json::Value &messages, const Json::Value &tools);

private:
    LLMConfig config_;
    std::shared_ptr<IChatHttpTransport> transport_;
    RetryDelay retryDelay_;
};
