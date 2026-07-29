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
    ASSERT_EQ(result.toolExecutions.size(), 1U);
    EXPECT_EQ(result.toolExecutions[0].tool, "known");
    EXPECT_TRUE(result.toolExecutions[0].success);
    EXPECT_GE(result.toolExecutions[0].durationMs, 0);
}
TEST(AgentLoopTest, ExecutesThreeToolChain) {
    FakeLLMClient fake{{toolCall("known"), toolCall("known"), toolCall("known"), answer("Combined")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "Combined"); EXPECT_EQ(result.toolsUsed.size(), 1U);
    EXPECT_EQ(result.toolExecutions.size(), 3U);
}
TEST(AgentLoopTest, CombinesDelayedFlightsActiveIncidentsAndWeather) {
    FakeLLMClient fake{{toolCall("find_delayed_flights"), toolCall("get_active_incidents"),
                        toolCall("get_latest_weather"), answer("Two delays, one active incident, clear weather.")}};
    std::vector<std::string> executed;
    auto operationsExecutor = [&](const std::string &name, const Json::Value &) {
        executed.push_back(name); Json::Value value; value["source"] = name; return value;
    };
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, operationsExecutor);
    EXPECT_EQ(executed, (std::vector<std::string>{"find_delayed_flights", "get_active_incidents", "get_latest_weather"}));
    EXPECT_EQ(result.toolsUsed.size(), 3U); EXPECT_NE(result.answer.find("weather"), std::string::npos);
}
TEST(AgentLoopTest, ExecutesIncidentManagementScenariosWithFakeProvider) {
    Json::Value severity; severity["severity"] = "CRITICAL";
    Json::Value search; search["query"] = "birds near the runway";
    Json::Value resolve; resolve["id"] = "12";
    const std::vector<std::pair<std::string, Json::Value>> scenarios{
        {"get_all_incidents", Json::Value(Json::objectValue)},
        {"get_incidents_by_severity", severity}, {"search_incidents", search}, {"resolve_incident", resolve}};
    for (const auto &[expectedName, expectedArgs] : scenarios) {
        FakeLLMClient fake{{toolCall(expectedName, expectedArgs), answer("Done")}};
        std::string calledName; Json::Value calledArgs;
        auto incidentExecutor = [&](const std::string &name, const Json::Value &args) {
            calledName = name; calledArgs = args; Json::Value result; result["ok"] = true; return result;
        };
        const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
            [&](const auto &messages, const auto &tools) { return fake.chat(messages, tools); }, incidentExecutor);
        ASSERT_EQ(result.toolsUsed.size(), 1U); EXPECT_EQ(result.toolsUsed[0], expectedName);
        EXPECT_EQ(calledName, expectedName); EXPECT_EQ(calledArgs, expectedArgs);
    }
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
TEST(AgentLoopTest, FindsAvailableGateBeforeAuthenticatedAssignment) {
    Json::Value flight; flight["flight_number"] = "SB2101";
    Json::Value assignment; assignment["flight_id"] = 21; assignment["gate_id"] = 3;
    FakeLLMClient fake{{toolCall("get_flight_by_number", flight), toolCall("get_available_gates"),
                        toolCall("assign_flight_to_gate", assignment), answer("SB2101 moved to A03.")}};
    std::vector<std::pair<std::string, Json::Value>> calls;
    auto gateExecutor = [&](const std::string &name, const Json::Value &args) {
        calls.emplace_back(name, args); Json::Value value;
        if (name == "get_flight_by_number") value["id"] = 21;
        else if (name == "get_available_gates") { Json::Value gate; gate["id"] = 3; gate["number"] = "A03"; value.append(gate); }
        else value["assigned"] = true;
        return value;
    };
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, gateExecutor);
    ASSERT_EQ(calls.size(), 3U); EXPECT_EQ(calls[2].first, "assign_flight_to_gate");
    EXPECT_EQ(calls[2].second["flight_id"], 21); EXPECT_EQ(calls[2].second["gate_id"], 3);
    EXPECT_EQ(result.toolsUsed.size(), 3U); EXPECT_NE(result.answer.find("A03"), std::string::npos);
}

TEST(AgentLoopTest, InvestigatesCriticalRunwayIncidentsWithoutWrites) {
    Json::Value severity; severity["severity"] = "CRITICAL";
    Json::Value query; query["query"] = "runway";
    FakeLLMClient fake{{toolCall("get_incidents_by_severity", severity),
                        toolCall("search_incidents", query), answer("Critical runway incident is active.")}};
    std::vector<std::string> calls;
    auto incidentExecutor = [&](const std::string &name, const Json::Value &) {
        calls.push_back(name); Json::Value value; value["status"] = "OPEN"; return value;
    };
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, incidentExecutor);
    EXPECT_EQ(calls, (std::vector<std::string>{"get_incidents_by_severity", "search_incidents"}));
    EXPECT_EQ(result.toolsUsed.size(), 2U); EXPECT_NE(result.answer.find("active"), std::string::npos);
}

TEST(AgentLoopTest, LooksUpRunwayBeforeAuthenticatedClosure) {
    Json::Value lookup; lookup["runway_code"] = "08L/26R";
    Json::Value update; update["runway_id"] = 1; update["status"] = "closed";
    auto first = toolCall("get_runway_by_code", lookup);
    auto second = toolCall("update_runway_status", update);
    first["choices"][0]["message"]["tool_calls"][0]["id"] = "runway-lookup";
    second["choices"][0]["message"]["tool_calls"][0]["id"] = "runway-update";
    FakeLLMClient fake{{first, second, answer("Runway 08L/26R is closed; SB2101 is affected.")}};
    std::vector<std::pair<std::string, Json::Value>> calls;
    auto runwayExecutor = [&](const std::string &name, const Json::Value &args) {
        calls.emplace_back(name, args); Json::Value result;
        if (name == "get_runway_by_code") result["id"] = 1;
        else { result["updated"] = true; result["affected_flights"].append("SB2101"); }
        return result;
    };
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, runwayExecutor);
    ASSERT_EQ(calls.size(), 2U); EXPECT_EQ(calls[0].second["runway_code"], "08L/26R");
    EXPECT_EQ(calls[1].second["runway_id"], 1); EXPECT_EQ(calls[1].second["status"], "closed");
    ASSERT_EQ(result.toolsUsed.size(), 2U); EXPECT_EQ(result.toolsUsed[0], "get_runway_by_code");
    EXPECT_EQ(result.toolsUsed[1], "update_runway_status"); EXPECT_FALSE(result.answer.empty());
}
TEST(AgentLoopTest, SendsMalformedArgumentsBackAsToolError) {
    FakeLLMClient fake{{toolCall("known", Json::Value("{broken")), answer("Recovered")}};
    auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); }, executor);
    EXPECT_EQ(result.answer, "Recovered");
    ASSERT_EQ(result.toolExecutions.size(), 1U);
    EXPECT_FALSE(result.toolExecutions[0].success);
    EXPECT_EQ(result.toolExecutions[0].result["code"], "invalid_arguments");
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
    EXPECT_TRUE(result.maxIterationsReached); EXPECT_EQ(result.toolsUsed.size(), 1U);
    EXPECT_EQ(result.toolExecutions.size(), 3U);
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

TEST(AgentLoopTest, ReturnsReplayableAssistantCallsResultsAndFinalMessage) {
    Json::Value args(Json::objectValue);
    auto call = toolCall("find_delayed_flights", args);
    call["choices"][0]["message"]["tool_calls"][0]["id"] = "call-delayed-1";
    FakeLLMClient fake{{call, answer("Delayed flights summarized")}};
    const auto result = AgentLoop::run(Json::Value(Json::arrayValue), Json::Value(Json::arrayValue),
        [&](const auto &m, const auto &t) { return fake.chat(m, t); },
        [](const std::string &, const Json::Value &) { Json::Value flights(Json::arrayValue), flight; flight["terminal"] = "A"; flights.append(flight); return flights; });
    ASSERT_EQ(result.generatedMessages.size(), 3U);
    EXPECT_EQ(result.generatedMessages[0]["tool_calls"][0]["id"], "call-delayed-1");
    EXPECT_EQ(result.generatedMessages[1]["tool_call_id"], "call-delayed-1");
    EXPECT_EQ(result.generatedMessages[2]["content"], "Delayed flights summarized");
}
