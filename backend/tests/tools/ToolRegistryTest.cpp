#include <gtest/gtest.h>
#include <set>
#include "agent/ToolRegistry.h"

TEST(ToolRegistryTest, RegistersEverySupportedToolExactlyOnce)
{
    const auto definitions = ToolRegistry::getToolDefinitions();
    ASSERT_TRUE(definitions.isArray());
    EXPECT_EQ(definitions.size(), 9U);
    std::set<std::string> names;
    for (const auto &definition : definitions)
        names.insert(definition["function"]["name"].asString());
    EXPECT_EQ(names.size(), definitions.size());
    EXPECT_TRUE(names.contains("find_delayed_flights"));
    EXPECT_TRUE(names.contains("create_incident"));
    EXPECT_TRUE(names.contains("resolve_incident"));
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
