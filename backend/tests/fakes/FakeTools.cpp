#include "tools/tools.h"

namespace {
Json::Value fakeResult(const std::string &name) {
    Json::Value value;
    value["fake_tool"] = name;
    return value;
}
}

Json::Value Tools::find_delayed_flights() { return fakeResult("find_delayed_flights"); }
Json::Value Tools::get_active_incidents() { return fakeResult("get_active_incidents"); }
Json::Value Tools::get_all_flights() { return fakeResult("get_all_flights"); }
Json::Value Tools::get_flight_details(const std::string &) { return fakeResult("get_flight_details"); }
Json::Value Tools::get_available_gates() { return fakeResult("get_available_gates"); }
Json::Value Tools::get_runway_status() { return fakeResult("get_runway_status"); }
Json::Value Tools::get_latest_weather() { return fakeResult("get_latest_weather"); }
Json::Value Tools::resolve_incident(const std::string &) { return fakeResult("resolve_incident"); }
Json::Value Tools::create_incident(const std::string &, const std::string &,
                                  const std::string &, const std::string &) {
    return fakeResult("create_incident");
}
