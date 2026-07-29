#include <gtest/gtest.h>
#include "services/agent_service.h"
#include "services/conversation_service.h"
#include "services/domain_error.h"
#include "services/flight_service.h"
#include "services/gate_service.h"
#include "services/incident_service.h"
#include "services/runway_service.h"
#include "services/terminal_service.h"
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
    FlightService::Dependencies dependencies; dependencies.all = [] { return arrayWith("id", "1"); };
    dependencies.delayed = [] { return arrayWith("status", "DELAYED"); }; dependencies.byId = [](const std::string &) { return found(); };
    FlightService service(dependencies);
    EXPECT_EQ(service.getAll().size(), 1U); EXPECT_EQ(service.getDelayed()[0]["status"], "DELAYED");
}
TEST(FlightServiceTest, ReturnsFlightAndRejectsInvalidOrMissingId) {
    FlightService::Dependencies dependencies; dependencies.all = [] { return Json::Value(); }; dependencies.delayed = [] { return Json::Value(); };
    dependencies.byId = [](const std::string &id) { if (id == "1") return found(); Json::Value v; v["found"] = false; return v; };
    FlightService service(dependencies);
    EXPECT_EQ(service.getById("1")["id"], "1");
    expectDomain(DomainErrorKind::Validation, [&] { service.getById("abc"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getById("2"); });
}
TEST(FlightServiceTest, PropagatesControlledRepositoryFailure) {
    FlightService::Dependencies dependencies; dependencies.all = []() -> Json::Value { throw std::runtime_error("offline"); };
    FlightService service(dependencies);
    EXPECT_THROW(service.getAll(), std::runtime_error);
}
TEST(FlightServiceTest, LooksUpNumberAndValidatesSearchStatus) {
    FlightService::Dependencies dependencies;
    dependencies.byNumber = [](const std::string &number) { Json::Value result; result["found"] = number == "sb2101"; result["flight"]["flight_number"] = "SB2101"; return result; };
    dependencies.search = [](const FlightSearchCriteria &criteria) { Json::Value result(Json::arrayValue), flight; flight["status"] = *criteria.status; result.append(flight); return result; };
    FlightService service(dependencies);
    EXPECT_EQ(service.getByNumber(" sb2101 ")["flight_number"], "SB2101");
    FlightSearchCriteria criteria; criteria.status = "delayed";
    EXPECT_EQ(service.searchFlights(criteria)[0]["status"], "DELAYED");
    criteria.status = "unknown"; expectDomain(DomainErrorKind::Validation, [&] { service.searchFlights(criteria); });
}
TEST(FlightServiceTest, UpdatesStatusAndMapsGateAssignmentConflicts) {
    FlightService::Dependencies dependencies;
    dependencies.byId = [](const std::string &id) { Json::Value result; result["found"] = id == "1"; result["flight"]["id"] = 1; return result; };
    dependencies.updateStatus = [](int id, const std::string &status) { return id == 1 && status == "DELAYED"; };
    dependencies.assignGate = [](int, int gate) { Json::Value result; result["outcome"] = gate == 2 ? "success" : "gate_unavailable"; result["new_gate"]["id"] = gate; return result; };
    FlightService service(dependencies);
    EXPECT_EQ(service.updateFlightStatus("1", "delayed")["id"], 1);
    EXPECT_EQ(service.assignFlightToGate("1", "2")["new_gate"]["id"], 2);
    expectDomain(DomainErrorKind::Conflict, [&] { service.assignFlightToGate("1", "3"); });
}
TEST(GateServiceTest, SeparatesAllAndAvailableQueries) {
    GateService::Dependencies dependencies; dependencies.all = [] { return arrayWith("status", "OCCUPIED"); };
    dependencies.available = [] { return arrayWith("status", "AVAILABLE"); }; GateService service(dependencies);
    EXPECT_EQ(service.getAll()[0]["status"], "OCCUPIED"); EXPECT_EQ(service.getAvailable()[0]["status"], "AVAILABLE");
}
TEST(GateServiceTest, ValidatesAndLooksUpGateIdentifiers) {
    GateService::Dependencies dependencies;
    dependencies.byId = [](int id) { Json::Value value; value["found"] = id == 3; value["gate"]["id"] = id; return value; };
    dependencies.byNumber = [](const std::string &number) { Json::Value value; value["found"] = number == "a03"; value["gate"]["gate_number"] = "A3"; return value; };
    dependencies.byTerminal = [](int id) { Json::Value values(Json::arrayValue); if (id == 1) values.append(Json::Value(Json::objectValue)); return values; };
    GateService service(dependencies);
    EXPECT_EQ(service.getGateById(3)["id"], 3);
    EXPECT_EQ(service.getGateByNumber(" a03 ")["gate_number"], "A3");
    EXPECT_EQ(service.getGatesByTerminal(1).size(), 1U);
    expectDomain(DomainErrorKind::Validation, [&] { service.getGateById(0); });
    expectDomain(DomainErrorKind::Validation, [&] { service.getGateByNumber("  "); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getGateById(4); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getGateByNumber("Z99"); });
}
TEST(GateServiceTest, CentralizesAvailabilityAndOperationalRules) {
    Json::Value gate; gate["status"] = "AVAILABLE";
    EXPECT_TRUE(GateService::isGateAvailable(gate)); EXPECT_TRUE(GateService::isGateOperational(gate));
    gate["status"] = "OCCUPIED";
    EXPECT_FALSE(GateService::isGateAvailable(gate)); EXPECT_TRUE(GateService::isGateOperational(gate));
    gate["status"] = "MAINTENANCE";
    EXPECT_FALSE(GateService::isGateAvailable(gate)); EXPECT_FALSE(GateService::isGateOperational(gate));
}

TEST(TerminalServiceTest, ReturnsTerminalsAndValidatesLookup) {
    Terminal first{1, "Terminal 1", "T1", 42000};
    TerminalService::Dependencies dependencies;
    dependencies.all = [first] { return std::vector<Terminal>{first}; };
    dependencies.byId = [first](int id) -> std::optional<Terminal> { if (id == 1) return first; return std::nullopt; };
    dependencies.byName = [first](const std::string &name) -> std::optional<Terminal> { if (name == "terminal 1") return first; return std::nullopt; };
    TerminalService service(dependencies);
    EXPECT_EQ(service.getAllTerminals().size(), 1U);
    EXPECT_EQ(service.getTerminalById(1).code, "T1");
    EXPECT_EQ(service.getTerminalByName(" terminal 1 ").id, 1);
    expectDomain(DomainErrorKind::Validation, [&] { service.getTerminalById(0); });
    expectDomain(DomainErrorKind::Validation, [&] { service.getTerminalByName(" "); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getTerminalById(2); });
}

TEST(TerminalServiceTest, ReturnsStatusAndTerminalFlightsIncludingEmptyList) {
    Terminal terminal{2, "Terminal 2", "T2", 28000};
    TerminalStatus status{terminal, 8, 5, 2, 1, 4};
    TerminalService::Dependencies dependencies;
    dependencies.byId = [terminal](int id) -> std::optional<Terminal> { if (id == 2) return terminal; return std::nullopt; };
    dependencies.status = [status](int id) -> std::optional<TerminalStatus> { if (id == 2) return status; return std::nullopt; };
    dependencies.flightsByTerminal = [](int) { return Json::Value(Json::arrayValue); };
    TerminalService service(dependencies);
    EXPECT_EQ(service.getTerminalStatus(2).availableGates, 5);
    EXPECT_TRUE(service.getFlightsByTerminal(2).empty());
    expectDomain(DomainErrorKind::NotFound, [&] { service.getFlightsByTerminal(3); });
}
TEST(RunwayServiceTest, PreservesEmptyResultsAndFailures) {
    EXPECT_TRUE(RunwayService([] { return Json::Value(Json::arrayValue); }).getStatus().empty());
    EXPECT_THROW(RunwayService([]() -> Json::Value { throw std::runtime_error("offline"); }).getStatus(), std::runtime_error);
}
TEST(RunwayServiceTest, ReturnsRunwayByIdAndCodeAndRejectsMissing) {
    RunwayService::Dependencies deps;
    deps.byId = [](int id) { Json::Value r; r["found"] = id == 1; r["runway"]["id"] = 1; r["runway"]["runway_code"] = "08L"; r["runway"]["status"] = "OPERATIONAL"; return r; };
    deps.byCode = [](const std::string &code) { Json::Value r; r["found"] = code == "08L"; r["runway"]["id"] = 1; r["runway"]["runway_code"] = "08L"; r["runway"]["status"] = "OPERATIONAL"; return r; };
    RunwayService service(deps);
    EXPECT_EQ(service.getById("1")["runway_code"], "08L");
    EXPECT_EQ(service.getByCode(" 08L ")["runway_code"], "08L");
    expectDomain(DomainErrorKind::Validation, [&] { service.getById("abc"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getById("2"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getByCode("99X"); });
}
TEST(RunwayServiceTest, UpdatesStatusNormalizesAndReportsAffectedFlights) {
    RunwayService::Dependencies deps;
    deps.byId = [](int id) { Json::Value r; r["found"] = id == 1; r["runway"]["id"] = 1; r["runway"]["runway_code"] = "08L"; r["runway"]["status"] = "OPERATIONAL"; return r; };
    deps.byCode = [](const std::string &code) { Json::Value r; r["found"] = code == "08L"; r["runway"]["id"] = 1; r["runway"]["runway_code"] = "08L"; r["runway"]["status"] = "OPERATIONAL"; return r; };
    std::string captured;
    deps.update = [&captured](int id, const std::string &status) { captured = status; return id == 1; };
    deps.affectedFlights = [](int) { Json::Value list(Json::arrayValue), f; f["flight_number"] = "SB2101"; f["status"] = "SCHEDULED"; f["origin"] = "TLV"; f["destination"] = "LHR"; list.append(f); f["flight_number"] = "SB2100"; f["status"] = "LANDED"; list.append(f); f["flight_number"] = "SB2199"; f["status"] = "CANCELLED"; list.append(f); return list; };
    RunwayService service(deps);
    auto result = service.updateStatus("1", "closed");
    EXPECT_TRUE(result["updated"].asBool());
    EXPECT_EQ(result["previous_status"], "OPERATIONAL");
    EXPECT_EQ(result["runway"]["status"], "CLOSED");
    EXPECT_EQ(result["affected_flight_count"].asInt(), 1);
    EXPECT_EQ(result["affected_flights"][0]["flight_number"], "SB2101");
    EXPECT_EQ(captured, "CLOSED");
    EXPECT_EQ(service.updateStatusByCode("08L", "Available")["runway"]["status"], "OPERATIONAL");
    expectDomain(DomainErrorKind::Validation, [&] { service.updateStatus("1", "flying"); });
}
TEST(IncidentServiceTest, ListsAndCreatesValidIncident) {
    IncidentService::Dependencies deps;
    deps.all = [] { return arrayWith("id", "1"); }; deps.active = [] { return arrayWith("status", "OPEN"); };
    deps.create = [](const std::string &title, const std::string &, const std::string &severity, const std::string &) { Json::Value v; v["title"] = title; v["severity"] = severity; return v; };
    IncidentService service(deps);
    EXPECT_EQ(service.getAll().size(), 1U); EXPECT_EQ(service.getActive()[0]["status"], "OPEN");
    EXPECT_EQ(service.create("Leak", "Details", "HIGH", "T1")["title"], "Leak");
    EXPECT_EQ(service.create(" Leak ", " Details ", "high", " T1 ")["severity"], "HIGH");
}
TEST(IncidentServiceTest, RejectsInvalidCreateInputs) {
    IncidentService::Dependencies deps; deps.create = [](const auto &, const auto &, const auto &, const auto &) { return Json::Value(); };
    IncidentService service(deps);
    expectDomain(DomainErrorKind::Validation, [&] { service.create("", "Details", "HIGH", ""); });
    expectDomain(DomainErrorKind::Validation, [&] { service.create("Leak", "Details", "VERY_HIGH", ""); });
}
TEST(IncidentServiceTest, ResolvesAndCentralizesResolutionErrors) {
    auto resolver = [](const std::string &id) { Json::Value v; v["found"] = id != "2"; v["already_resolved"] = id == "3"; v["incident"]["id"] = id; return v; };
    IncidentService::Dependencies deps; deps.resolve = resolver; IncidentService service(deps);
    EXPECT_EQ(service.resolve("1")["id"], "1");
    expectDomain(DomainErrorKind::Validation, [&] { service.resolve("bad"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.resolve("2"); });
    expectDomain(DomainErrorKind::Conflict, [&] { service.resolve("3"); });
}
TEST(IncidentServiceTest, FiltersSearchesAndLooksUpIncidents) {
    std::string severitySeen, querySeen;
    IncidentService::Dependencies deps;
    deps.bySeverity = [&](const std::string &severity) { severitySeen = severity; return Json::Value(Json::arrayValue); };
    deps.search = [&](const std::string &query) { querySeen = query; return Json::Value(Json::arrayValue); };
    deps.byId = [](const std::string &id) { Json::Value value; if (id == "1") value["id"] = id; return value; };
    IncidentService service(deps);
    EXPECT_TRUE(service.getBySeverity(" critical ").empty()); EXPECT_EQ(severitySeen, "CRITICAL");
    EXPECT_TRUE(service.search(" bird ").empty()); EXPECT_EQ(querySeen, "bird");
    EXPECT_EQ(service.getById("1")["id"], "1");
    expectDomain(DomainErrorKind::Validation, [&] { service.getBySeverity("urgent"); });
    expectDomain(DomainErrorKind::Validation, [&] { service.search("  "); });
    expectDomain(DomainErrorKind::Validation, [&] { service.getById("0"); });
    expectDomain(DomainErrorKind::NotFound, [&] { service.getById("2"); });
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
    expectDomain(DomainErrorKind::NotFound, [&] { service.loadOwnedMessages("9", "8"); });
}
TEST(AgentServiceTest, HandlesNewAndOwnedConversationAndPersistsVisibleTurns) {
    std::vector<std::string> roles; Json::Value history = arrayWith("role", "assistant"); history[0]["content"] = "Earlier";
    auto runner = [](Json::Value messages, const ToolExecutionContext &context) { AgentLoop::Result result; result.answer = "All clear"; result.toolsUsed.append("get_runway_status"); EXPECT_GE(messages.size(), 2U); EXPECT_TRUE(context.authenticated); EXPECT_EQ(context.userId, "7"); return result; };
    AgentService service(fakeConversations(history, true, &roles), runner);
    auto fresh = service.query("7", "status", std::nullopt); EXPECT_EQ(fresh.conversationId, "9"); EXPECT_EQ(fresh.toolsUsed.size(), 1U);
    auto existing = service.query("7", "again", std::string("5")); EXPECT_EQ(existing.conversationId, "5");
    EXPECT_EQ(roles.size(), 4U);
}
TEST(AgentServiceTest, RejectsNonOwnerAndDoesNotSaveFalseProviderSuccess) {
    std::vector<std::string> roles;
    AgentService denied(fakeConversations(Json::Value(Json::arrayValue), false, &roles), [](Json::Value, const ToolExecutionContext &) { return AgentLoop::Result{}; });
    expectDomain(DomainErrorKind::NotFound, [&] { denied.query("7", "x", std::string("5")); });
    AgentService failed(fakeConversations(Json::Value(Json::arrayValue), true, &roles), [](Json::Value, const ToolExecutionContext &) { AgentLoop::Result r; r.providerFailed = true; return r; });
    expectDomain(DomainErrorKind::ProviderUnavailable, [&] { failed.query("7", "x", std::nullopt); });
    ASSERT_FALSE(roles.empty()); EXPECT_EQ(roles.back(), "user");
}

TEST(ConversationServiceTest, GeneratesDeterministicNormalizedTitles) {
    EXPECT_EQ(ConversationService::titleFromFirstMessage("  Full\n operations   status  "), "Full operations status");
    EXPECT_EQ(ConversationService::titleFromFirstMessage(" \t "), "New conversation");
    const auto title = ConversationService::titleFromFirstMessage(std::string(100, 'a'));
    EXPECT_EQ(title.size(), 80U); EXPECT_EQ(title.substr(77), "...");
}

TEST(ConversationServiceTest, ReplaysCompleteBoundedStructuredTurnsAndLegacyRows) {
    Json::Value rows(Json::arrayValue);
    auto add = [&](const char *role, const char *content, const char *turn, const Json::Value &payload = Json::Value()) {
        Json::Value row; row["role"] = role; row["content"] = content;
        if (*turn) row["turn_id"] = turn; if (!payload.isNull()) row["provider_payload"] = payload;
        rows.append(row);
    };
    add("user", "old", ""); add("assistant", "old answer", "");
    Json::Value user; user["role"] = "user"; user["content"] = "Which flights are delayed?";
    add("user", user["content"].asCString(), "turn-2", user);
    Json::Value call; call["role"] = "assistant"; call["content"] = Json::nullValue;
    call["tool_calls"][0]["id"] = "call-delayed-1"; call["tool_calls"][0]["function"]["name"] = "find_delayed_flights";
    add("assistant", "", "turn-2", call);
    Json::Value tool; tool["role"] = "tool"; tool["tool_call_id"] = "call-delayed-1"; tool["content"] = "[{\"terminal\":\"A\"}]";
    add("tool", tool["content"].asCString(), "turn-2", tool);
    Json::Value final; final["role"] = "assistant"; final["content"] = "One delayed flight";
    add("assistant", final["content"].asCString(), "turn-2", final);
    auto service = ConversationService({
        [](const std::string &) { return Json::Value(); }, {},
        [](const std::string &, const std::string &) { return true; }, {},
        [rows](const std::string &, const std::string &) { return rows; }});
    const auto replay = service.loadReplayHistory("1", "7", 1);
    ASSERT_EQ(replay.size(), 4U); EXPECT_EQ(replay[1]["tool_calls"][0]["id"], "call-delayed-1");
    EXPECT_EQ(replay[2]["tool_call_id"], "call-delayed-1");
}

TEST(ConversationServiceTest, BoundsHistoryConfiguration) {
    unsetenv("AGENT_HISTORY_MAX_TURNS"); EXPECT_EQ(ConversationService::historyMaxTurnsFromEnvironment(), 30U);
    setenv("AGENT_HISTORY_MAX_TURNS", "0", 1); EXPECT_EQ(ConversationService::historyMaxTurnsFromEnvironment(), 1U);
    setenv("AGENT_HISTORY_MAX_TURNS", "1000", 1); EXPECT_EQ(ConversationService::historyMaxTurnsFromEnvironment(), 100U);
    setenv("AGENT_HISTORY_MAX_TURNS", "bad", 1); EXPECT_EQ(ConversationService::historyMaxTurnsFromEnvironment(), 30U);
    unsetenv("AGENT_HISTORY_MAX_TURNS");
}
