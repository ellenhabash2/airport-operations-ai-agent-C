#include "LLMClient.h"
#include <drogon/drogon.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

namespace {
constexpr std::size_t kMaximumResponseBytes = 4 * 1024 * 1024;

Json::Value controlledError(const std::string &category,
                            const std::string &message,
                            int statusCode = 0)
{
    Json::Value error;
    error["error"] = message;
    error["error_category"] = category;
    if (statusCode > 0) error["provider_status"] = statusCode;
    return error;
}

bool isTemporaryStatus(int status)
{
    return status == 429 || status == 500 || status == 502 ||
           status == 503 || status == 504;
}

Json::Value statusError(int status)
{
    switch (status) {
        case 400: return controlledError("bad_request", "AI provider rejected the request", status);
        case 401: return controlledError("authentication", "AI provider authentication failed", status);
        case 403: return controlledError("access_denied", "AI provider access was denied", status);
        case 404: return controlledError("model_configuration", "Configured AI model is unavailable", status);
        case 429: return controlledError("rate_limit", "AI provider quota or rate limit was reached", status);
        default: return controlledError("provider_unavailable", "AI provider is temporarily unavailable", status);
    }
}

bool splitHttpsUrl(const std::string &url, std::string &origin, std::string &path)
{
    constexpr const char *scheme = "https://";
    if (url.rfind(scheme, 0) != 0) return false;
    const auto slash = url.find('/', 8);
    origin = slash == std::string::npos ? url : url.substr(0, slash);
    path = slash == std::string::npos ? "/" : url.substr(slash);
    return origin.size() > 8;
}

class DrogonChatHttpTransport final : public IChatHttpTransport
{
public:
    ChatHttpResponse post(const std::string &url,
                          const std::map<std::string, std::string> &headers,
                          const std::string &body,
                          int timeoutSeconds) override
    {
        std::string origin, path;
        if (!splitHttpsUrl(url, origin, path)) return {};

        // Modern TLS only; certificate verification must remain enabled.
        auto client = drogon::HttpClient::newHttpClient(origin, nullptr, false, true);
        auto request = drogon::HttpRequest::newHttpRequest();
        request->setMethod(drogon::Post);
        request->setPath(path);
        request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        request->setBody(body);
        for (const auto &[name, value] : headers) request->addHeader(name, value);

        const auto [result, response] = client->sendRequest(request, timeoutSeconds);
        ChatHttpResponse output;
        output.networkOk = result == drogon::ReqResult::Ok && response != nullptr;
        output.timedOut = result == drogon::ReqResult::Timeout;
        if (response) {
            output.statusCode = response->getStatusCode();
            output.body = std::string(response->getBody());
        }
        return output;
    }
};
}

LLMClient::LLMClient() : LLMClient(LLMConfig::fromEnvironment()) {}

LLMClient::LLMClient(LLMConfig config,
                     std::shared_ptr<IChatHttpTransport> transport,
                     RetryDelay retryDelay)
    : config_(std::move(config)),
      transport_(transport ? std::move(transport) : std::make_shared<DrogonChatHttpTransport>()),
      retryDelay_(retryDelay ? std::move(retryDelay) : [](int attempt) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500 * attempt));
      })
{
}

std::string LLMClient::ask(const std::string &question)
{
    Json::Value messages(Json::arrayValue), message;
    message["role"] = "user";
    message["content"] = question;
    messages.append(message);
    const auto response = chatWithTools(messages, Json::Value(Json::arrayValue));
    if (response.isMember("error")) return "ERROR: " + response["error"].asString();
    return response["choices"][0]["message"]["content"].asString();
}

Json::Value LLMClient::chatWithTools(const Json::Value &messages, const Json::Value &tools)
{
    if (const auto error = config_.validationError(); !error.empty())
        return controlledError("configuration", error);

    Json::Value body;
    body["model"] = config_.model;
    body["messages"] = messages;
    if (tools.isArray() && !tools.empty()) {
        body["tools"] = tools;
        body["tool_choice"] = "auto";
    }
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    const std::string serialized = Json::writeString(writer, body);
    const std::map<std::string, std::string> headers{
        {"Authorization", "Bearer " + config_.apiKey},
        {"Content-Type", "application/json"}
    };

    for (int attempt = 1; attempt <= config_.maxAttempts; ++attempt) {
        const auto response = transport_->post(
            config_.baseUrl, headers, serialized, config_.requestTimeoutSeconds);

        if (!response.networkOk) {
            std::cerr << "AI provider=gemini category="
                      << (response.timedOut ? "timeout" : "network")
                      << " attempt=" << attempt << std::endl;
            if (attempt < config_.maxAttempts) { retryDelay_(attempt); continue; }
            return controlledError(response.timedOut ? "timeout" : "network",
                                   response.timedOut ? "AI provider request timed out"
                                                     : "AI provider connection failed");
        }

        if (response.statusCode != 200) {
            std::cerr << "AI provider=gemini status=" << response.statusCode
                      << " category=" << (isTemporaryStatus(response.statusCode) ? "temporary" : "request")
                      << " attempt=" << attempt << std::endl;
            if (isTemporaryStatus(response.statusCode) && attempt < config_.maxAttempts) {
                retryDelay_(attempt); continue;
            }
            return statusError(response.statusCode);
        }
        if (response.body.empty()) return controlledError("empty_response", "AI provider returned an empty response");
        if (response.body.size() > kMaximumResponseBytes)
            return controlledError("response_too_large", "AI provider response exceeded the allowed size");

        Json::Value parsed;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        auto reader = std::unique_ptr<Json::CharReader>(readerBuilder.newCharReader());
        if (!reader->parse(response.body.data(), response.body.data() + response.body.size(),
                           &parsed, &parseError))
            return controlledError("malformed_response", "AI provider returned malformed JSON");
        if (!parsed.isMember("choices") || !parsed["choices"].isArray() || parsed["choices"].empty())
            return controlledError("malformed_response", "AI provider response did not include choices");
        if (!parsed["choices"][0].isMember("message") || !parsed["choices"][0]["message"].isObject())
            return controlledError("malformed_response", "AI provider response did not include a message");
        return parsed;
    }
    return controlledError("provider_unavailable", "AI provider is temporarily unavailable");
}
