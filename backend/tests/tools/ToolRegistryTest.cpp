#include <gtest/gtest.h>
#include <set>
#include "agent/ToolRegistry.h"

TEST(ToolRegistryTest, RegistersEverySupportedToolExactlyOnce)
{
    const auto definitions = ToolRegistry::getToolDefinitions();
    ASSERT_TRUE(definitions.isArray());
    EXPECT_EQ(definitions.size(), 22U);
    std::set<std::string> names;
    for (const auto &definition : definitions)
        names.insert(definition["function"]["name"].asString());
    EXPECT_EQ(names.size(), definitions.size());
    EXPECT_TRUE(names.contains("find_delayed_flights"));
    EXPECT_TRUE(names.contains("create_incident"));
    EXPECT_TRUE(names.contains("resolve_incident"));
    EXPECT_TRUE(names.contains("get_all_incidents"));
    EXPECT_TRUE(names.contains("get_incidents_by_severity"));
    EXPECT_TRUE(names.contains("search_incidents"));
}

TEST(ToolRegistryTest, RegistersExpandedIncidentToolsWithValidatedSchemas)
{
    for (const auto &definition : ToolRegistry::getToolDefinitions()) {
        const auto name = definition["function"]["name"].asString();
        if (name == "get_incidents_by_severity") {
            const auto &severity = definition["function"]["parameters"]["properties"]["severity"];
            EXPECT_EQ(severity["enum"].size(), 4U); EXPECT_EQ(severity["enum"][3], "CRITICAL");
        }
    }
    EXPECT_EQ(ToolRegistry::executeTool("get_all_incidents", Json::Value(Json::objectValue))["fake_tool"], "get_all_incidents");
    Json::Value severity; severity["severity"] = "HIGH";
    EXPECT_EQ(ToolRegistry::executeTool("get_incidents_by_severity", severity)["fake_tool"], "get_incidents_by_severity");
    Json::Value query; query["query"] = "bird";
    EXPECT_EQ(ToolRegistry::executeTool("search_incidents", query)["fake_tool"], "search_incidents");
}


TEST(ToolRegistryTest, RegistersGateAndTerminalOperationsTools)
{
    const auto definitions = ToolRegistry::getToolDefinitions();
    const std::set<std::string> expected{
        "get_all_gates", "get_gate_by_id", "get_gate_by_number", "get_available_gates",
        "get_terminal_status", "get_flights_by_terminal"};
    std::set<std::string> found;
    for (const auto &definition : definitions) {
        const auto name = definition["function"]["name"].asString();
        if (expected.contains(name)) found.insert(name);
        if (name == "get_gate_by_id") {
            EXPECT_EQ(definition["function"]["parameters"]["properties"]["gate_id"]["type"], "integer");
            EXPECT_EQ(definition["function"]["parameters"]["required"][0], "gate_id");
        }
        if (name == "get_terminal_status" || name == "get_flights_by_terminal") {
            EXPECT_EQ(definition["function"]["parameters"]["properties"]["terminal_id"]["type"], "integer");
            EXPECT_EQ(definition["function"]["parameters"]["required"][0], "terminal_id");
        }
    }
    EXPECT_EQ(found, expected);

    Json::Value gateArgs; gateArgs["gate_id"] = 1;
    EXPECT_EQ(ToolRegistry::executeTool("get_gate_by_id", gateArgs)["fake_tool"], "get_gate_by_id");
    Json::Value terminalArgs; terminalArgs["terminal_id"] = 1;
    EXPECT_EQ(ToolRegistry::executeTool("get_terminal_status", terminalArgs)["fake_tool"], "get_terminal_status");
}

TEST(ToolRegistryTest, RegistersCompleteFlightOperationsTools)
{
    const auto definitions = ToolRegistry::getToolDefinitions();
    for (const std::string name : {"get_flight_by_id", "get_flight_by_number", "search_flights", "update_flight_status", "assign_flight_to_gate"}) {
        bool found = false;
        for (const auto &definition : definitions) if (definition["function"]["name"].asString() == name) found = true;
        EXPECT_TRUE(found) << name;
    }
    Json::Value args; args["flight_id"] = 1;
    EXPECT_EQ(ToolRegistry::executeTool("get_flight_by_id", args)["fake_tool"], "get_flight_by_id");
}

TEST(ToolRegistryTest, ExposesRequiredArgumentSchema)
{
    for (const auto &definition : ToolRegistry::getToolDefinitions()) {
        if (definition["function"]["name"].asString() == "get_flight_details") {
            const auto &parameters = definition["function"]["parameters"];
            EXPECT_EQ(parameters["properties"]["id"]["type"].asString(), "string");
            ASSERT_EQ(parameters["required"].size(), 1U);
            EXPECT_EQ(parameters["required"][0].asString(), "id");
            return;
        }
    }
    FAIL() << "get_flight_details was not registered";
}

TEST(ToolRegistryTest, RegistersRunwayReadAndWriteTools)
{
    const auto definitions = ToolRegistry::getToolDefinitions();
    const std::set<std::string> expected{
        "get_runway_status", "get_runway_by_id", "get_runway_by_code", "update_runway_status"};
    std::set<std::string> found;
    for (const auto &definition : definitions) {
        const auto name = definition["function"]["name"].asString();
        if (expected.contains(name)) found.insert(name);
        if (name == "get_runway_by_id") {
            EXPECT_EQ(definition["function"]["parameters"]["properties"]["runway_id"]["type"], "integer");
            EXPECT_EQ(definition["function"]["parameters"]["required"][0], "runway_id");
        }
        if (name == "update_runway_status")
            EXPECT_EQ(definition["function"]["parameters"]["required"][0], "status");
    }
    EXPECT_EQ(found, expected);

    Json::Value byCode; byCode["runway_code"] = "08L";
    EXPECT_EQ(ToolRegistry::executeTool("get_runway_by_code", byCode)["fake_tool"], "get_runway_by_code");
    Json::Value update; update["runway_code"] = "08L"; update["status"] = "closed";
    EXPECT_EQ(ToolRegistry::executeTool("update_runway_status", update)["fake_tool"], "update_runway_status");
}

TEST(ToolRegistryTest, RejectsUnknownToolWithoutDatabaseAccess)
{
    const auto result = ToolRegistry::executeTool("not_a_real_tool", Json::Value(Json::objectValue));
    EXPECT_EQ(result["error"].asString(), "Unknown tool: not_a_real_tool");
}

TEST(ToolRegistryTest, ExecutesKnownToolThroughFakeWithoutDatabaseAccess)
{
    const auto result = ToolRegistry::executeTool("find_delayed_flights", Json::Value(Json::objectValue));
    EXPECT_EQ(result["fake_tool"].asString(), "find_delayed_flights");
}