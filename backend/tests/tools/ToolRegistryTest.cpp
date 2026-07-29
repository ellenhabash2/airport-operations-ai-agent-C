#include <gtest/gtest.h>
#include <set>
#include "agent/ToolRegistry.h"

namespace
{
const std::set<std::string> canonicalTools{
    "get_all_flights", "find_delayed_flights", "get_flight_by_id", "get_flight_by_number",
    "search_flights", "update_flight_status", "assign_flight_to_gate", "get_all_gates",
    "get_available_gates", "get_gate_by_id", "get_gate_by_number", "get_terminal_status",
    "get_flights_by_terminal", "get_runway_status", "get_runway_by_id", "get_runway_by_code",
    "update_runway_status", "get_latest_weather", "get_all_incidents", "get_active_incidents",
    "get_incidents_by_severity", "search_incidents", "create_incident", "resolve_incident"};

ToolExecutionContext authenticated()
{
    ToolExecutionContext context;
    context.authenticated = true;
    context.userId = "7";
    context.conversationId = "12";
    return context;
}
}

TEST(ToolRegistryTest, RegistersExactCanonicalInventoryAndLegacyAliasOnce)
{
    const auto definitions = ToolRegistry::getToolDefinitions();
    ASSERT_TRUE(definitions.isArray());
    EXPECT_EQ(definitions.size(), canonicalTools.size() + 1);
    std::set<std::string> names;
    for (const auto &definition : definitions) names.insert(definition["function"]["name"].asString());
    EXPECT_EQ(names.size(), definitions.size());
    for (const auto &name : canonicalTools) EXPECT_TRUE(names.contains(name)) << name;
    EXPECT_TRUE(names.contains("get_flight_details"));
}

TEST(ToolRegistryTest, MetadataIsCompleteDeterministicAndProviderDerived)
{
    const auto listed = ToolRegistry::operational().listTools();
    const auto schemas = ToolRegistry::operational().providerToolSchemas();
    ASSERT_EQ(listed.size(), schemas.size());
    std::string previous;
    for (Json::ArrayIndex index = 0; index < schemas.size(); ++index) {
        const auto &item = listed[index];
        EXPECT_FALSE(item.name.empty()); EXPECT_FALSE(item.description.empty()); EXPECT_TRUE(item.handler);
        EXPECT_EQ(item.parameters["type"], "object"); EXPECT_TRUE(item.parameters["properties"].isObject());
        EXPECT_EQ(schemas[index]["function"]["name"], item.name);
        EXPECT_GE(item.name, previous); previous = item.name;
    }
    EXPECT_EQ(ToolRegistry::operational().findTool("update_flight_status")->access, ToolAccess::Write);
    EXPECT_EQ(ToolRegistry::operational().findTool("find_delayed_flights")->access, ToolAccess::ReadOnly);
    EXPECT_TRUE(ToolRegistry::operational().findTool("get_flight_details")->deprecated);
}

TEST(ToolRegistryTest, RejectsDuplicateRegistration)
{
    ToolRegistry registry;
    auto schema = Json::Value(Json::objectValue); schema["type"] = "object";
    schema["properties"] = Json::Value(Json::objectValue); schema["required"] = Json::Value(Json::arrayValue);
    ToolDefinition tool{"same", "description", schema, ToolAccess::ReadOnly,
                        [](const auto &, const auto &) { return Json::Value(); }};
    registry.registerTool(tool);
    EXPECT_THROW(registry.registerTool(tool), std::invalid_argument);
}

TEST(ToolRegistryTest, SchemasMatchRequiredArgumentsAndDomainEnums)
{
    const auto &registry = ToolRegistry::operational();
    EXPECT_EQ(registry.findTool("get_flight_by_id")->parameters["required"][0], "flight_id");
    EXPECT_EQ(registry.findTool("get_gate_by_number")->parameters["required"][0], "gate_number");
    EXPECT_EQ(registry.findTool("get_terminal_status")->parameters["required"][0], "terminal_id");
    EXPECT_EQ(registry.findTool("get_runway_by_code")->parameters["required"][0], "runway_code");
    EXPECT_EQ(registry.findTool("resolve_incident")->parameters["properties"]["id"]["type"], "integer");
    EXPECT_EQ(registry.findTool("get_incidents_by_severity")->parameters["properties"]["severity"]["enum"].size(), 4U);
    EXPECT_EQ(registry.findTool("update_runway_status")->parameters["properties"]["status"]["enum"].size(), 3U);
}

TEST(ToolRegistryTest, RejectsUnknownAndInvalidArgumentsSafely)
{
    auto result = ToolRegistry::executeTool("not_a_real_tool", Json::Value(Json::objectValue));
    EXPECT_EQ(result["error"]["code"], "unknown_tool");
    result = ToolRegistry::executeTool("get_flight_by_id", Json::Value(Json::objectValue));
    EXPECT_EQ(result["error"]["code"], "invalid_arguments");
    Json::Value invalid; invalid["flight_id"] = -2;
    result = ToolRegistry::executeTool("get_flight_by_id", invalid);
    EXPECT_EQ(result["error"]["code"], "invalid_arguments");
    Json::Value wrong; wrong["severity"] = "URGENT";
    result = ToolRegistry::executeTool("get_incidents_by_severity", wrong);
    EXPECT_EQ(result["error"]["code"], "invalid_arguments");
}

TEST(ToolRegistryTest, ExecutesReadToolsAndLegacyAliasThroughSameRegistry)
{
    EXPECT_EQ(ToolRegistry::executeTool("find_delayed_flights", Json::Value(Json::objectValue))["fake_tool"], "find_delayed_flights");
    Json::Value canonical; canonical["flight_id"] = 5;
    EXPECT_EQ(ToolRegistry::executeTool("get_flight_by_id", canonical)["fake_tool"], "get_flight_by_id");
    Json::Value legacy; legacy["id"] = "5";
    EXPECT_EQ(ToolRegistry::executeTool("get_flight_details", legacy)["fake_tool"], "get_flight_details");
}

TEST(ToolRegistryTest, EveryWriteToolRequiresAuthenticatedIdentity)
{
    const std::set<std::string> writes{"update_flight_status", "assign_flight_to_gate", "update_runway_status", "create_incident", "resolve_incident"};
    for (const auto &name : writes) {
        ASSERT_EQ(ToolRegistry::operational().findTool(name)->access, ToolAccess::Write);
        const auto result = ToolRegistry::executeTool(name, Json::Value(Json::objectValue));
        EXPECT_EQ(result["error"]["code"], "unauthenticated") << name;
    }
}

TEST(ToolRegistryTest, AuthenticatedContextPermitsValidWriteDispatch)
{
    Json::Value status; status["flight_id"] = 1; status["status"] = "delayed";
    EXPECT_EQ(ToolRegistry::executeTool("update_flight_status", status, authenticated())["fake_tool"], "update_flight_status");
    Json::Value assignment; assignment["flight_number"] = "SB2101"; assignment["gate_number"] = "A03";
    EXPECT_EQ(ToolRegistry::executeTool("assign_flight_to_gate", assignment, authenticated())["fake_tool"], "assign_flight_to_gate");
    Json::Value runway; runway["runway_code"] = "08L/26R"; runway["status"] = "closed";
    EXPECT_EQ(ToolRegistry::executeTool("update_runway_status", runway, authenticated())["fake_tool"], "update_runway_status");
    Json::Value incident; incident["title"] = "Bird activity"; incident["description"] = "Near runway"; incident["severity"] = "HIGH";
    EXPECT_EQ(ToolRegistry::executeTool("create_incident", incident, authenticated())["fake_tool"], "create_incident");
    Json::Value resolve; resolve["id"] = 12;
    EXPECT_EQ(ToolRegistry::executeTool("resolve_incident", resolve, authenticated())["fake_tool"], "resolve_incident");
}
