#include <gtest/gtest.h>
#include "agent/PresentationService.h"

namespace {
ToolExecutionRecord execution(const std::string &tool, Json::Value result,
                              bool success = true, std::size_t sequence = 0)
{
    ToolExecutionRecord record;
    record.tool = tool;
    record.result = std::move(result);
    record.success = success;
    record.sequence = sequence;
    return record;
}

Json::Value entity(int id = 1)
{
    Json::Value value;
    value["id"] = id;
    value["gate_id"] = Json::nullValue;
    return value;
}

Json::Value list(int id = 1)
{
    Json::Value value(Json::arrayValue);
    value.append(entity(id));
    return value;
}

std::string typeFor(const std::vector<ToolExecutionRecord> &records)
{
    const auto presentation = PresentationService::generate(records);
    return presentation ? (*presentation)["type"].asString() : "";
}
}

TEST(PresentationServiceTest, ValidatesAllContractsAndRejectsMalformedContracts)
{
    Json::Value presentation, data;
    presentation["type"] = "flight_list"; data["flights"] = Json::Value(Json::arrayValue);
    presentation["data"] = data; EXPECT_TRUE(PresentationService::validate(presentation));
    presentation["type"] = "unknown"; EXPECT_FALSE(PresentationService::validate(presentation));
    presentation["type"] = "flight_list"; presentation.removeMember("data");
    EXPECT_FALSE(PresentationService::validate(presentation));
    presentation["data"] = Json::Value(Json::arrayValue);
    EXPECT_FALSE(PresentationService::validate(presentation));

    presentation["data"] = Json::Value(Json::objectValue);
    presentation["type"] = "flight_status"; presentation["data"]["flight"] = entity();
    EXPECT_TRUE(PresentationService::validate(presentation));
    presentation["type"] = "gate_assignment";
    presentation["data"]["new_gate"] = entity(2);
    presentation["data"]["previous_gate"] = Json::nullValue;
    EXPECT_TRUE(PresentationService::validate(presentation));
    presentation["type"] = "runway_status";
    presentation["data"] = Json::Value(Json::objectValue);
    presentation["data"]["runways"] = Json::Value(Json::arrayValue);
    presentation["data"]["affected_flights"] = Json::Value(Json::arrayValue);
    EXPECT_TRUE(PresentationService::validate(presentation));
    presentation["type"] = "incident_list";
    presentation["data"] = Json::Value(Json::objectValue);
    presentation["data"]["incidents"] = Json::Value(Json::arrayValue);
    EXPECT_TRUE(PresentationService::validate(presentation));
    presentation["type"] = "operations_overview";
    presentation["data"] = Json::Value(Json::objectValue);
    presentation["data"]["delayed_flights"] = Json::Value(Json::arrayValue);
    presentation["data"]["active_incidents"] = Json::Value(Json::arrayValue);
    presentation["data"]["weather"] = Json::Value(Json::objectValue);
    EXPECT_TRUE(PresentationService::validate(presentation));
}

TEST(PresentationServiceTest, MapsEveryFlightListTool)
{
    for (const char *tool : {"get_all_flights", "find_delayed_flights", "search_flights", "get_flights_by_terminal"})
        EXPECT_EQ(typeFor({execution(tool, list())}), "flight_list") << tool;
}

TEST(PresentationServiceTest, MapsEveryFlightStatusToolIncludingLegacyWrapper)
{
    for (const char *tool : {"get_flight_by_id", "get_flight_by_number", "update_flight_status"})
        EXPECT_EQ(typeFor({execution(tool, entity())}), "flight_status") << tool;
    Json::Value legacy; legacy["found"] = true; legacy["flight"] = entity(7);
    const auto presentation = PresentationService::generate({execution("get_flight_details", legacy)});
    ASSERT_TRUE(presentation); EXPECT_EQ((*presentation)["data"]["flight"]["id"], 7);
}

TEST(PresentationServiceTest, MapsGateRunwayAndIncidentTools)
{
    Json::Value assignment; assignment["flight"] = entity(); assignment["previous_gate"] = Json::nullValue;
    assignment["new_gate"] = entity(2);
    auto gate = PresentationService::generate({execution("assign_flight_to_gate", assignment)});
    ASSERT_TRUE(gate); EXPECT_EQ((*gate)["type"], "gate_assignment");
    EXPECT_TRUE((*gate)["data"]["previous_gate"].isNull());

    for (const char *tool : {"get_runway_status", "get_runway_by_id", "get_runway_by_code"}) {
        const Json::Value result = std::string(tool) == "get_runway_status" ? list() : entity();
        EXPECT_EQ(typeFor({execution(tool, result)}), "runway_status") << tool;
    }
    Json::Value update; update["runway"] = entity(); update["affected_flights"] = Json::Value(Json::arrayValue);
    EXPECT_EQ(typeFor({execution("update_runway_status", update)}), "runway_status");

    for (const char *tool : {"get_all_incidents", "get_active_incidents", "get_incidents_by_severity", "search_incidents"})
        EXPECT_EQ(typeFor({execution(tool, list())}), "incident_list") << tool;
    EXPECT_EQ(typeFor({execution("create_incident", entity())}), "incident_list");
    Json::Value resolved; resolved["incident"] = entity(4); resolved["found"] = true;
    auto incident = PresentationService::generate({execution("resolve_incident", resolved)});
    ASSERT_TRUE(incident); EXPECT_EQ((*incident)["data"]["incidents"][0]["id"], 4);
}

TEST(PresentationServiceTest, OverviewIsOrderIndependentAllowsExtrasAndUsesLatestSuccess)
{
    Json::Value weather; weather["condition"] = "CLEAR";
    std::vector<ToolExecutionRecord> records{
        execution("get_latest_weather", weather, true, 0),
        execution("find_delayed_flights", list(1), true, 1),
        execution("get_all_gates", list(9), true, 2),
        execution("get_active_incidents", list(2), true, 3),
        execution("find_delayed_flights", list(3), true, 4)};
    const auto presentation = PresentationService::generate(records);
    ASSERT_TRUE(presentation); EXPECT_EQ((*presentation)["type"], "operations_overview");
    ASSERT_EQ((*presentation)["data"]["delayed_flights"].size(), 1U);
    EXPECT_EQ((*presentation)["data"]["delayed_flights"][0]["id"], 3);
}

TEST(PresentationServiceTest, UsesDocumentedPrecedenceAndPartialOverviewFallback)
{
    Json::Value weather; weather["condition"] = "CLEAR";
    Json::Value assignment; assignment["flight"] = entity(); assignment["new_gate"] = entity(2);
    std::vector<ToolExecutionRecord> complete{
        execution("assign_flight_to_gate", assignment), execution("find_delayed_flights", list()),
        execution("get_active_incidents", list()), execution("get_latest_weather", weather)};
    EXPECT_EQ(typeFor(complete), "operations_overview");
    complete.pop_back();
    EXPECT_EQ(typeFor(complete), "gate_assignment");
}

TEST(PresentationServiceTest, MalformedFailedAndNoToolResultsProduceNoFalsePresentation)
{
    EXPECT_FALSE(PresentationService::generate({}));
    EXPECT_FALSE(PresentationService::generate({execution("find_delayed_flights", entity())}));
    EXPECT_FALSE(PresentationService::generate({execution("assign_flight_to_gate", entity(), false)}));
    Json::Value missing; missing["found"] = false;
    EXPECT_FALSE(PresentationService::generate({execution("get_flight_details", missing)}));
}

TEST(PresentationServiceTest, RepeatedListsUseLatestAndPreserveIntegerIdsNullsAndEmptyArrays)
{
    auto presentation = PresentationService::generate({
        execution("find_delayed_flights", list(1), true, 0),
        execution("find_delayed_flights", Json::Value(Json::arrayValue), true, 1)});
    ASSERT_TRUE(presentation);
    EXPECT_TRUE((*presentation)["data"]["flights"].isArray());
    EXPECT_TRUE((*presentation)["data"]["flights"].empty());
    presentation = PresentationService::generate({execution("find_delayed_flights", list(8))});
    ASSERT_TRUE(presentation);
    EXPECT_TRUE((*presentation)["data"]["flights"][0]["id"].isIntegral());
    EXPECT_TRUE((*presentation)["data"]["flights"][0]["gate_id"].isNull());
}
