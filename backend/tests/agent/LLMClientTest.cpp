#include <gtest/gtest.h>
#include <deque>
#include <memory>
#include "agent/LLMClient.h"

namespace {
class FakeTransport final : public IChatHttpTransport
{
public:
    std::deque<ChatHttpResponse> responses;
    int calls{0};
    std::string url, body;
    std::map<std::string, std::string> headers;
    int timeout{0};

    ChatHttpResponse post(const std::string &requestUrl,
                          const std::map<std::string, std::string> &requestHeaders,
                          const std::string &requestBody,
                          int requestTimeout) override {
        ++calls; url = requestUrl; headers = requestHeaders;
        body = requestBody; timeout = requestTimeout;
        auto response = responses.front(); responses.pop_front(); return response;
    }
};

LLMConfig config() {
    LLMConfig value;
    value.apiKey = "unit-test-key";
    value.model = "configured-gemini-model";
    value.baseUrl = "https://gemini.example/v1/chat/completions";
    value.maxAttempts = 3;
    return value;
}
ChatHttpResponse http(int status, const std::string &body = "{}") {
    return {true, false, status, body};
}
std::string textResponse(const std::string &content = "All clear") {
    Json::Value value;
    value["choices"][0]["message"]["role"] = "assistant";
    value["choices"][0]["message"]["content"] = content;
    Json::StreamWriterBuilder writer; writer["indentation"] = "";
    return Json::writeString(writer, value);
}
Json::Value parse(const std::string &text) {
    Json::Value value; Json::CharReaderBuilder builder; std::string errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    EXPECT_TRUE(reader->parse(text.data(), text.data() + text.size(), &value, &errors));
    return value;
}
std::unique_ptr<LLMClient> client(const std::shared_ptr<FakeTransport> &transport) {
    return std::make_unique<LLMClient>(config(), transport, [](int) {});
}
Json::Value messages() {
    Json::Value list(Json::arrayValue), message;
    message["role"] = "user"; message["content"] = "Airport status?"; list.append(message);
    return list;
}
Json::Value tools() {
    Json::Value list(Json::arrayValue), tool;
    tool["type"] = "function"; tool["function"]["name"] = "get_runway_status"; list.append(tool);
    return list;
}
}

TEST(LLMClientTest, SendsConfiguredGeminiRequest)
{
    auto transport = std::make_shared<FakeTransport>();
    transport->responses.push_back(http(200, textResponse()));
    client(transport)->chatWithTools(messages(), tools());
    EXPECT_EQ(transport->url, "https://gemini.example/v1/chat/completions");
    EXPECT_EQ(transport->headers["Authorization"], "Bearer unit-test-key");
    EXPECT_EQ(transport->headers["Content-Type"], "application/json");
    const auto request = parse(transport->body);
    EXPECT_EQ(request["model"].asString(), "configured-gemini-model");
    EXPECT_EQ(request["messages"], messages());
    EXPECT_EQ(request["tools"], tools());
    EXPECT_EQ(request["tool_choice"].asString(), "auto");
}

TEST(LLMClientTest, OmitsToolFieldsWhenNoToolsAreProvided)
{
    auto transport = std::make_shared<FakeTransport>();
    transport->responses.push_back(http(200, textResponse()));
    client(transport)->chatWithTools(messages(), Json::Value(Json::arrayValue));
    const auto request = parse(transport->body);
    EXPECT_FALSE(request.isMember("tools")); EXPECT_FALSE(request.isMember("tool_choice"));
}

TEST(LLMClientTest, ParsesNormalTextResponse)
{
    auto transport = std::make_shared<FakeTransport>();
    transport->responses.push_back(http(200, textResponse("Operations normal")));
    const auto response = client(transport)->chatWithTools(messages(), tools());
    EXPECT_EQ(response["choices"][0]["message"]["content"].asString(), "Operations normal");
}

TEST(LLMClientTest, PreservesOneAndMultipleToolCalls)
{
    Json::Value response;
    for (int index = 1; index <= 2; ++index) {
        Json::Value call;
        call["id"] = "gemini-call-" + std::to_string(index);
        call["type"] = "function";
        call["function"]["name"] = "tool_" + std::to_string(index);
        call["function"]["arguments"] = "{\"id\":\"" + std::to_string(index) + "\"}";
        response["choices"][0]["message"]["tool_calls"].append(call);
    }
    Json::StreamWriterBuilder writer; writer["indentation"] = "";
    auto transport = std::make_shared<FakeTransport>();
    transport->responses.push_back(http(200, Json::writeString(writer, response)));
    const auto parsed = client(transport)->chatWithTools(messages(), tools());
    const auto &calls = parsed["choices"][0]["message"]["tool_calls"];
    ASSERT_EQ(calls.size(), 2U);
    EXPECT_EQ(calls[0]["id"].asString(), "gemini-call-1");
    EXPECT_EQ(calls[1]["function"]["arguments"].asString(), "{\"id\":\"2\"}");
}

class ProviderStatusTest : public testing::TestWithParam<std::tuple<int, const char *>> {};
TEST_P(ProviderStatusTest, MapsNonRetryableStatusToControlledError)
{
    auto [status, category] = GetParam();
    auto transport = std::make_shared<FakeTransport>();
    transport->responses.push_back(http(status, "provider details must not escape"));
    const auto result = client(transport)->chatWithTools(messages(), tools());
    EXPECT_EQ(result["error_category"].asString(), category);
    EXPECT_EQ(result["provider_status"].asInt(), status);
    EXPECT_EQ(transport->calls, 1);
    EXPECT_EQ(result.toStyledString().find("provider details"), std::string::npos);
}
INSTANTIATE_TEST_SUITE_P(Errors, ProviderStatusTest, testing::Values(
    std::tuple{400, "bad_request"}, std::tuple{401, "authentication"},
    std::tuple{403, "access_denied"}, std::tuple{404, "model_configuration"}));

TEST(LLMClientTest, RetriesRateLimitAndTemporaryServerFailures)
{
    for (const int status : {429, 503}) {
        auto transport = std::make_shared<FakeTransport>();
        transport->responses = {http(status), http(status), http(200, textResponse())};
        const auto result = client(transport)->chatWithTools(messages(), tools());
        EXPECT_FALSE(result.isMember("error")); EXPECT_EQ(transport->calls, 3);
    }
}

TEST(LLMClientTest, ConvertsTimeoutAndNetworkFailureToControlledErrors)
{
    for (const bool timedOut : {true, false}) {
        auto transport = std::make_shared<FakeTransport>();
        transport->responses = {{false, timedOut, 0, ""}, {false, timedOut, 0, ""}, {false, timedOut, 0, ""}};
        const auto result = client(transport)->chatWithTools(messages(), tools());
        EXPECT_EQ(result["error_category"].asString(), timedOut ? "timeout" : "network");
        EXPECT_EQ(transport->calls, 3);
    }
}

TEST(LLMClientTest, RejectsMalformedAndIncompleteResponses)
{
    const std::vector<std::pair<std::string, std::string>> cases{
        {"not json", "malformed_response"}, {"{}", "malformed_response"},
        {"{\"choices\":[{}]}", "malformed_response"}, {"", "empty_response"}};
    for (const auto &[body, category] : cases) {
        auto transport = std::make_shared<FakeTransport>();
        transport->responses.push_back(http(200, body));
        const auto result = client(transport)->chatWithTools(messages(), tools());
        EXPECT_EQ(result["error_category"].asString(), category);
    }
}

TEST(LLMClientTest, DoesNotRequireEnvironmentApiKeyWhenConfiguredForTest)
{
    unsetenv("GEMINI_API_KEY");
    auto transport = std::make_shared<FakeTransport>();
    transport->responses.push_back(http(200, textResponse()));
    EXPECT_FALSE(client(transport)->chatWithTools(messages(), tools()).isMember("error"));
}
