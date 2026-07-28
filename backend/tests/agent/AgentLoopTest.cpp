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
TEST(AgentLoopTest, ResolvesFlightTerminalAndOtherFlightsWithThreeTools) {
    Json::Value flightArgs; flightArgs["flight_number"] = "SB2101";
    Json::Value terminalArgs; terminalArgs["terminal_id"] = 2;
    auto flightCall = toolCall("get_flight_by_number", flightArgs);
    auto statusCall = toolCall("get_terminal_status", terminalArgs);
    auto flightsCall = toolCall("get_flights_by_terminal", terminalArgs);
    flightCall["choices"][0]["message"]["tool_calls"][0]["id"] = "flight-call";
    statusCall["choices"][0]["message"]["tool_calls"][0]["id"] = "status-call";
    flightsCall["choices"][0]["message"]["tool_calls"][0]["id"] = "flights-call";
    FakeLLMClient fake{{flightCall, statusCall, flightsCall,
                        answer("SB2101 is at gate C3 in T2; five gates are available and two other flights operate there.")}};
    std::vector<std::pair<std::string, Json::Value>> calls;
    auto terminalExecutor = [&](const std::string &name, const Json::Value &args) {
        calls.emplace_back(name, args);
        Json::Value result;
        if (name == "get_flight_by_number") { result["terminal_id"] = 2; result["gate"] = "C3"; }
        else if (name == "get_terminal_status") result["available_gates"] = 5;
        else result.append("SB2102");
        return result;
    };
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &messages, const auto &tools) { return fake.chat(messages, tools); }, terminalExecutor);
    ASSERT_EQ(calls.size(), 3U);
    EXPECT_EQ(calls[0].first, "get_flight_by_number"); EXPECT_EQ(calls[0].second["flight_number"], "SB2101");
    EXPECT_EQ(calls[1].first, "get_terminal_status"); EXPECT_EQ(calls[1].second["terminal_id"], 2);
    EXPECT_EQ(calls[2].first, "get_flights_by_terminal"); EXPECT_EQ(calls[2].second["terminal_id"], 2);
    ASSERT_EQ(result.toolsUsed.size(), 3U);
    EXPECT_EQ(result.toolsUsed[0], "get_flight_by_number");
    EXPECT_EQ(result.toolsUsed[1], "get_terminal_status");
    EXPECT_EQ(result.toolsUsed[2], "get_flights_by_terminal");
    EXPECT_NE(result.answer.find("SB2101"), std::string::npos);
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
    FakeLLMClient fake{{toolCall("known"), toolCall("known"), toolCall("known"), answer("Forced summary")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor, 3);
    EXPECT_TRUE(result.maxIterationsReached); EXPECT_EQ(result.toolsUsed.size(), 3U);
    EXPECT_EQ(result.answer, "Forced summary");
}

TEST(AgentLoopTest, PreservesAssistantCallAndMatchingToolResultId) {
    Json::Value first = toolCall("known", Json::Value("{\"id\":\"42\"}"));
    first["choices"][0]["message"]["tool_calls"][0]["id"] = "gemini-call-42";
    Json::Value messagesSeenBySecondCall;
    int callCount = 0;
    auto provider = [&](const Json::Value &messages, const Json::Value &) {
        if (++callCount == 1) return first;
        messagesSeenBySecondCall = messages;
        return answer("Done");
    };
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
                                       provider, executor);
    ASSERT_EQ(messagesSeenBySecondCall.size(), 2U);
    EXPECT_EQ(messagesSeenBySecondCall[0]["tool_calls"][0]["id"].asString(), "gemini-call-42");
    EXPECT_EQ(messagesSeenBySecondCall[0]["tool_calls"][0]["function"]["arguments"].asString(),
              "{\"id\":\"42\"}");
    EXPECT_EQ(messagesSeenBySecondCall[1]["role"].asString(), "tool");
    EXPECT_EQ(messagesSeenBySecondCall[1]["tool_call_id"].asString(), "gemini-call-42");
    EXPECT_EQ(result.answer, "Done");
}

TEST(AgentLoopTest, PreservesSeveralCallsFromOneAssistantMessage) {
    Json::Value first = toolCall("known");
    Json::Value secondCall = first["choices"][0]["message"]["tool_calls"][0];
    secondCall["id"] = "call-2";
    first["choices"][0]["message"]["tool_calls"].append(secondCall);
    Json::Value nextMessages;
    int callCount = 0;
    auto provider = [&](const Json::Value &messages, const Json::Value &) {
        if (++callCount == 1) return first;
        nextMessages = messages;
        return answer("Both complete");
    };
    AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue), provider, executor);
    ASSERT_EQ(nextMessages.size(), 3U);
    EXPECT_EQ(nextMessages[1]["tool_call_id"].asString(), "call-1");
    EXPECT_EQ(nextMessages[2]["tool_call_id"].asString(), "call-2");
}
