#include <gtest/gtest.h>
#include "services/agent_service.h"
#include "services/conversation_service.h"
#include "services/domain_error.h"
#include "services/flight_service.h"
#include "services/gate_service.h"
#include "services/incident_service.h"
#include "services/runway_service.h"
#include "services/weather_service.h"
#include <stdexcept>

namespace {
Json::Value arrayWith(const char *key, const char *value) { Json::Value list(Json::arrayValue), item; item[key] = value; list.append(item); return list; }
Json::Value found(const char *key = "id", const char *value = "1") { Json::Value result; result["found"] = true; result["flight"][key] = value; return result; }
template<class F> void expectDomain(DomainErrorKind kind, F operation) {
    try { operation(); FAIL() << "expected DomainError"; } catch (const DomainError &error) { EXPECT_EQ(error.kind(), kind); }
}
ConversationService fakeConversations(Json::Value history, bool owns, std::vector<std::string> *roles = nullptr) {
    return ConversationService({
        [](const std::string &) { Json::Value v; v["id"] = "9"; return v; },
        [roles](const std::string &, const std::string &role, const std::string &) { if (roles) roles->push_back(role); Json::Value v; v["id"] = "1"; return v; },
        [owns](const std::string &, const std::string &) { return owns; },
        [](const std::string &) { return Json::Value(Json::arrayValue); },
        [history](const std::string &, const std::string &) { return history; }});
}
}

TEST(FlightServiceTest, ReturnsAllAndDelayedFlights) {
    FlightService service({[] { return arrayWith("id", "1"); }, [] { return arrayWith("status", "DELAYED"); }, [](const std::string &) { return found(); }});
    EXPECT_EQ(service.getAll().size(), 1U); EXPECT_EQ(service.getDelayed()[0]["status"], "DELAYED");
}
TEST(FlightServiceTest, ReturnsFlightAndRejectsInvalidOrMissingId) {
    FlightService service({[] { return Json::Value(); }, [] { return Json::Value(); }, [](const std::string &id) { if (id == "1") return found(); Json::Value v; v["found"] = false; return v; }});
    EXPECT_EQ(service.getById("1")["id"], "1");
    expectDomain(DomainErrorKind::Validation, [&] { service.getById("abc"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getById("2"); });
}
TEST(FlightServiceTest, PropagatesControlledRepositoryFailure) {
    FlightService service({[]() -> Json::Value { throw std::runtime_error("offline"); }, [] { return Json::Value(); }, [](const std::string &) { return Json::Value(); }});
    EXPECT_THROW(service.getAll(), std::runtime_error);
}
TEST(GateServiceTest, SeparatesAllAndAvailableQueries) {
    GateService service({[] { return arrayWith("status", "OCCUPIED"); }, [] { return arrayWith("status", "AVAILABLE"); }});
    EXPECT_EQ(service.getAll()[0]["status"], "OCCUPIED"); EXPECT_EQ(service.getAvailable()[0]["status"], "AVAILABLE");
}
TEST(RunwayServiceTest, PreservesEmptyResultsAndFailures) {
    EXPECT_TRUE(RunwayService([] { return Json::Value(Json::arrayValue); }).getStatus().empty());
    EXPECT_THROW(RunwayService([]() -> Json::Value { throw std::runtime_error("offline"); }).getStatus(), std::runtime_error);
}
TEST(IncidentServiceTest, ListsAndCreatesValidIncident) {
    IncidentService service({[] { return arrayWith("id", "1"); }, [] { return arrayWith("status", "OPEN"); },
        [](const std::string &title, const std::string &, const std::string &, const std::string &) { Json::Value v; v["title"] = title; return v; },
        [](const std::string &) { return Json::Value(); }});
    EXPECT_EQ(service.getAll().size(), 1U); EXPECT_EQ(service.getActive()[0]["status"], "OPEN");
    EXPECT_EQ(service.create("Leak", "Details", "HIGH", "T1")["title"], "Leak");
}
TEST(IncidentServiceTest, RejectsInvalidCreateInputs) {
    IncidentService service({{}, {}, [](const auto &, const auto &, const auto &, const auto &) { return Json::Value(); }, {}});
    expectDomain(DomainErrorKind::Validation, [&] { service.create("", "Details", "HIGH", ""); });
    expectDomain(DomainErrorKind::Validation, [&] { service.create("Leak", "Details", "VERY_HIGH", ""); });
}
TEST(IncidentServiceTest, ResolvesAndCentralizesResolutionErrors) {
    auto resolver = [](const std::string &id) { Json::Value v; v["found"] = id != "2"; v["already_resolved"] = id == "3"; v["incident"]["id"] = id; return v; };
    IncidentService service({{}, {}, {}, resolver});
    EXPECT_EQ(service.resolve("1")["id"], "1");
    expectDomain(DomainErrorKind::Validation, [&] { service.resolve("bad"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.resolve("2"); });
    expectDomain(DomainErrorKind::Conflict, [&] { service.resolve("3"); });
}
TEST(WeatherServiceTest, ReadsAndCreatesValidWeather) {
    WeatherService service({[] { return arrayWith("condition", "CLEAR"); }, [] { Json::Value v; v["condition"] = "CLEAR"; return v; },
        [](const std::string &condition, float, float, float) { Json::Value v; v["condition"] = condition; return v; }});
    EXPECT_EQ(service.getRecent().size(), 1U); EXPECT_EQ(service.getLatest()["condition"], "CLEAR");
    EXPECT_EQ(service.create("RAIN", 2, 3, 4)["condition"], "RAIN");
    expectDomain(DomainErrorKind::Validation, [&] { service.create("", 2, 3, 4); });
}
TEST(ConversationServiceTest, CreatesListsLoadsAndSavesRoles) {
    std::vector<std::string> roles; auto service = fakeConversations(arrayWith("role", "user"), true, &roles);
    EXPECT_EQ(service.create("7"), "9"); EXPECT_TRUE(service.list("7").empty());
    EXPECT_EQ(service.loadOwnedMessages("9", "7").size(), 1U);
    service.saveUserMessage("9", "hello"); service.saveAssistantMessage("9", "hi");
    EXPECT_EQ(roles, (std::vector<std::string>{"user", "assistant"}));
}
TEST(ConversationServiceTest, RejectsNonOwner) {
    auto service = fakeConversations(Json::Value(Json::arrayValue), false);
    expectDomain(DomainErrorKind::Forbidden, [&] { service.loadOwnedMessages("9", "8"); });
}
TEST(AgentServiceTest, HandlesNewAndOwnedConversationAndPersistsVisibleTurns) {
    std::vector<std::string> roles; Json::Value history = arrayWith("role", "assistant"); history[0]["content"] = "Earlier";
    auto runner = [](Json::Value messages) { AgentLoop::Result result; result.answer = "All clear"; result.toolsUsed.append("get_runway_status"); EXPECT_GE(messages.size(), 2U); return result; };
    AgentService service(fakeConversations(history, true, &roles), runner);
    auto fresh = service.query("7", "status", std::nullopt); EXPECT_EQ(fresh.conversationId, "9"); EXPECT_EQ(fresh.toolsUsed.size(), 1U);
    auto existing = service.query("7", "again", std::string("5")); EXPECT_EQ(existing.conversationId, "5");
    EXPECT_EQ(roles.size(), 4U);
}
TEST(AgentServiceTest, RejectsNonOwnerAndDoesNotSaveFalseProviderSuccess) {
    std::vector<std::string> roles;
    AgentService denied(fakeConversations(Json::Value(Json::arrayValue), false, &roles), [](Json::Value) { return AgentLoop::Result{}; });
    expectDomain(DomainErrorKind::Forbidden, [&] { denied.query("7", "x", std::string("5")); });
    AgentService failed(fakeConversations(Json::Value(Json::arrayValue), true, &roles), [](Json::Value) { AgentLoop::Result r; r.providerFailed = true; return r; });
    expectDomain(DomainErrorKind::ProviderUnavailable, [&] { failed.query("7", "x", std::nullopt); });
    ASSERT_FALSE(roles.empty()); EXPECT_EQ(roles.back(), "user");
}
