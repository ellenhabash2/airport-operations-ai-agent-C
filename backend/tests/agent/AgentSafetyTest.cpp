#include <gtest/gtest.h>
#include "agent/AgentSafety.h"

TEST(AgentSafetyTest, RecursivelyRedactsSensitiveArgumentsWithoutMutatingInput)
{
    Json::Value arguments;
    arguments["flight_id"] = 10;
    arguments["Authorization"] = "Bearer synthetic-token";
    arguments["nested"]["apiKey"] = "synthetic-api-key";
    arguments["nested"]["provider_headers"]["x"] = "secret";
    arguments["items"][0]["database_url"] = "postgresql://synthetic";
    arguments["items"][1]["connection_string"] = "host=synthetic";
    arguments["runway_key"] = "redact-me";
    const auto sanitized = AgentSafety::sanitizeArguments(arguments);
    EXPECT_EQ(sanitized["flight_id"], 10);
    EXPECT_EQ(sanitized["Authorization"], "[redacted]");
    EXPECT_EQ(sanitized["nested"]["apiKey"], "[redacted]");
    EXPECT_EQ(sanitized["nested"]["provider_headers"], "[redacted]");
    EXPECT_EQ(sanitized["items"][0]["database_url"], "[redacted]");
    EXPECT_EQ(sanitized["items"][1]["connection_string"], "[redacted]");
    EXPECT_EQ(sanitized["runway_key"], "[redacted]");
    EXPECT_EQ(arguments["Authorization"], "Bearer synthetic-token");
}

TEST(AgentSafetyTest, SerializesOrderedSuccessAndSafeFailureMetadataOnly)
{
    ToolExecutionRecord success;
    success.tool = "find_delayed_flights"; success.callId = "call-1";
    success.arguments["status"] = "DELAYED"; success.result["private"] = "raw-result";
    success.durationMs = -1; success.sequence = 0;
    ToolExecutionRecord failure;
    failure.tool = "assign_flight_to_gate"; failure.success = false;
    failure.arguments["password"] = "synthetic-password";
    failure.result["error"] = "SELECT secret FROM users /src/file.cpp stack trace";
    failure.result["code"] = "gate_unavailable"; failure.durationMs = 3; failure.sequence = 1;
    const auto publicRecords = AgentSafety::toolExecutionsJson({success, failure});
    ASSERT_EQ(publicRecords.size(), 2U);
    EXPECT_EQ(publicRecords[0]["tool"], "find_delayed_flights");
    EXPECT_EQ(publicRecords[0]["duration_ms"], 0);
    EXPECT_FALSE(publicRecords[0].isMember("result"));
    EXPECT_EQ(publicRecords[1]["status"], "error");
    EXPECT_EQ(publicRecords[1]["arguments"]["password"], "[redacted]");
    EXPECT_EQ(publicRecords[1]["error_code"], "conflict");
    const auto rendered = publicRecords.toStyledString();
    EXPECT_EQ(rendered.find("SELECT secret"), std::string::npos);
    EXPECT_EQ(rendered.find("stack trace"), std::string::npos);
    EXPECT_EQ(rendered.find("synthetic-password"), std::string::npos);
}

TEST(AgentSafetyTest, MapsOnlyDocumentedErrorCategories)
{
    const std::vector<std::pair<std::string, std::string>> cases{
        {"invalid_arguments", "validation_error"}, {"flight_not_found", "not_found"},
        {"unauthorized", "unauthorized"}, {"forbidden", "forbidden"},
        {"gate_unavailable", "conflict"}, {"provider_failure", "provider_error"},
        {"timeout", "timeout"}, {"sqlstate_23505", "internal_error"}};
    for (const auto &[code, expected] : cases) {
        Json::Value result; result["error"] = "synthetic internal detail"; result["code"] = code;
        EXPECT_EQ(AgentSafety::safeErrorCode(result), expected) << code;
    }
}
