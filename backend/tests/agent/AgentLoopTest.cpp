#include <gtest/gtest.h>
#include <deque>
#include "agent/AgentLoop.h"

namespace {
Json::Value answer(const std::string &text) {
    Json::Value response;
    response["choices"][0]["message"]["content"] = text;
    return response;
}
Json::Value toolCall(const std::string &name, const Json::Value &arguments = Json::Value(Json::objectValue)) {
    Json::Value response, call;
    call["id"] = "call-1";
    call["function"]["name"] = name;
    call["function"]["arguments"] = arguments;
    response["choices"][0]["message"]["tool_calls"].append(call);
    return response;
}
class FakeLLMClient {
public:
    std::deque<Json::Value> responses;
    Json::Value chat(const Json::Value &, const Json::Value &) {
        auto response = responses.front(); responses.pop_front(); return response;
    }
};
auto executor = [](const std::string &name, const Json::Value &) {
    Json::Value result;
    if (name == "known") result["ok"] = true;
    else result["error"] = "Unknown tool: " + name;
    return result;
};
}

TEST(AgentLoopTest, ReturnsResponseWithoutTools) {
    FakeLLMClient fake{{answer("All clear")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "All clear"); EXPECT_TRUE(result.toolsUsed.empty());
}
TEST(AgentLoopTest, ExecutesOneTool) {
    FakeLLMClient fake{{toolCall("known"), answer("Done")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "Done"); ASSERT_EQ(result.toolsUsed.size(), 1U);
}
TEST(AgentLoopTest, ExecutesThreeToolChain) {
    FakeLLMClient fake{{toolCall("known"), toolCall("known"), toolCall("known"), answer("Combined")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "Combined"); EXPECT_EQ(result.toolsUsed.size(), 3U);
}
TEST(AgentLoopTest, SendsMalformedArgumentsBackAsToolError) {
    FakeLLMClient fake{{toolCall("known", Json::Value("{broken")), answer("Recovered")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "Recovered");
}
TEST(AgentLoopTest, AllowsModelToRecoverFromUnknownTool) {
    FakeLLMClient fake{{toolCall("unknown"), answer("Unknown tool explained")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "Unknown tool explained");
}
TEST(AgentLoopTest, ReportsProviderFailure) {
    Json::Value failure; failure["error"] = "offline"; FakeLLMClient fake{{failure}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_TRUE(result.providerFailed);
}
TEST(AgentLoopTest, StopsAtMaximumIterations) {
    FakeLLMClient fake{{toolCall("known"), toolCall("known"), toolCall("known")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor, 3);
    EXPECT_TRUE(result.maxIterationsReached); EXPECT_EQ(result.toolsUsed.size(), 3U);
}
