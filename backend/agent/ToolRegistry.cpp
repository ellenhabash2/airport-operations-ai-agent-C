#include "ToolRegistry.h"
#include "tools/tools.h"

namespace
{
// Helper: build one tool definition in OpenAI format.
Json::Value makeTool(const std::string &name, const std::string &description)
{
    Json::Value fn;
    fn["name"] = name;
    fn["description"] = description;
    Json::Value params;
    params["type"] = "object";
    params["properties"] = Json::Value(Json::objectValue);
    params["required"] = Json::Value(Json::arrayValue);
    fn["parameters"] = params;

    Json::Value tool;
    tool["type"] = "function";
    tool["function"] = fn;
    return tool;
}

// Helper: a tool that takes a single string parameter.
Json::Value makeToolWithParam(const std::string &name, const std::string &description,
                              const std::string &paramName, const std::string &paramDesc)
{
    Json::Value tool = makeTool(name, description);
    Json::Value prop;
    prop["type"] = "string";
    prop["description"] = paramDesc;
    tool["function"]["parameters"]["properties"][paramName] = prop;
    tool["function"]["parameters"]["required"].append(paramName);
    return tool;
}

Json::Value property(const char *type, const char *description)
{
    Json::Value value; value["type"] = type; value["description"] = description; return value;
}

Json::Value flightStatusProperty()
{
    auto value = property("string", "Flight status");
    for (const char *status : {"scheduled", "boarding", "in_flight", "delayed", "cancelled", "landed"}) value["enum"].append(status);
    return value;
}
}

Json::Value ToolRegistry::getToolDefinitions()
{
    Json::Value tools(Json::arrayValue);

    // Read tools
    tools.append(makeTool("find_delayed_flights", "Returns all flights that are currently delayed."));
    tools.append(makeTool("get_active_incidents", "Returns airport operational incidents that still need attention (status OPEN or INVESTIGATING; excludes RESOLVED)."));
    tools.append(makeTool("get_all_flights", "Returns the full list of flights."));
    tools.append(makeToolWithParam("get_flight_details", "Returns full details of a single flight by its numeric id.",
                                   "id", "The numeric flight id, e.g. \"42\"."));
    {
        auto tool = makeTool("get_flight_by_id", "Returns a flight by its internal numeric identifier.");
        tool["function"]["parameters"]["properties"]["flight_id"] = property("integer", "Internal numeric flight identifier");
        tool["function"]["parameters"]["required"].append("flight_id"); tools.append(tool);
    }
    {
        auto tool = makeTool("get_flight_by_number", "Returns a flight by its public flight number.");
        tool["function"]["parameters"]["properties"]["flight_number"] = property("string", "Public flight number, for example SB2101");
        tool["function"]["parameters"]["required"].append("flight_number"); tools.append(tool);
    }
    {
        auto tool = makeTool("search_flights", "Searches flights using optional filters which are combined with AND.");
        auto &props = tool["function"]["parameters"]["properties"];
        props["origin"] = property("string", "Partial origin airport code or name");
        props["destination"] = property("string", "Partial destination airport code or name");
        props["status"] = flightStatusProperty();
        props["airline"] = property("string", "Partial airline name");
        props["terminal_id"] = property("integer", "Numeric terminal identifier"); tools.append(tool);
    }
    tools.append(makeTool("get_all_gates", "Returns every gate with terminal and operational status."));
    {
        auto tool = makeTool("get_gate_by_id", "Returns a gate by its internal numeric identifier.");
        tool["function"]["parameters"]["properties"]["gate_id"] = property("integer", "Internal numeric gate identifier");
        tool["function"]["parameters"]["required"].append("gate_id"); tools.append(tool);
    }
    {
        auto tool = makeTool("get_gate_by_number", "Returns a gate by its public gate number.");
        tool["function"]["parameters"]["properties"]["gate_number"] = property("string", "Public gate number, for example A03");
        tool["function"]["parameters"]["required"].append("gate_number"); tools.append(tool);
    }
    tools.append(makeTool("get_available_gates", "Returns gates that are currently available (status AVAILABLE)."));
    tools.append(makeTool("get_runway_status", "Returns all runways and their status."));
    tools.append(makeTool("get_latest_weather", "Returns the latest weather report for the airport."));

    // Action tools
    tools.append(makeToolWithParam("resolve_incident", "Marks an incident as resolved by its numeric id.",
                                   "id", "The numeric incident id, e.g. \"7\"."));
    {
        auto tool = makeTool("update_flight_status", "Updates a flight to a supported operational status.");
        auto &params = tool["function"]["parameters"];
        params["properties"]["flight_id"] = property("integer", "Internal numeric flight identifier");
        params["properties"]["status"] = flightStatusProperty();
        params["required"].append("flight_id"); params["required"].append("status"); tools.append(tool);
    }
    {
        auto tool = makeTool("assign_flight_to_gate", "Transactionally assigns a flight to a gate; public numbers may be used directly.");
        auto &props = tool["function"]["parameters"]["properties"];
        props["flight_id"] = property("integer", "Internal flight identifier");
        props["flight_number"] = property("string", "Public flight number");
        props["gate_id"] = property("integer", "Internal gate identifier");
        props["gate_number"] = property("string", "Public gate number, for example A03");
        tools.append(tool);
    }

    // create_incident takes several parameters
    {
        Json::Value tool = makeTool("create_incident", "Creates a new airport operational incident.");
        Json::Value &props = tool["function"]["parameters"]["properties"];
        Json::Value s; s["type"] = "string";
        s["description"] = "Short incident title.";        props["title"] = s;
        s["description"] = "Detailed description.";        props["description"] = s;
        s["description"] = "Location, e.g. Terminal 1.";   props["location"] = s;

        // severity is constrained to the four values the schema accepts.
        Json::Value sev; sev["type"] = "string";
        sev["description"] = "Severity level.";
        sev["enum"].append("LOW");
        sev["enum"].append("MEDIUM");
        sev["enum"].append("HIGH");
        sev["enum"].append("CRITICAL");
        props["severity"] = sev;
        tool["function"]["parameters"]["required"].append("title");
        tool["function"]["parameters"]["required"].append("description");
        tool["function"]["parameters"]["required"].append("severity");
        tools.append(tool);
    }

    return tools;
}

Json::Value ToolRegistry::executeTool(const std::string &name, const Json::Value &args)
{
    if (name == "find_delayed_flights")   return Tools::find_delayed_flights();
    if (name == "get_active_incidents")   return Tools::get_active_incidents();
    if (name == "get_all_flights")        return Tools::get_all_flights();
    if (name == "get_flight_details")     return Tools::get_flight_details(args.get("id", "").asString());
    if (name == "get_flight_by_id")       return Tools::get_flight_by_id(args["flight_id"].isInt() ? std::to_string(args["flight_id"].asInt()) : "");
    if (name == "get_flight_by_number")   return Tools::get_flight_by_number(args["flight_number"].isString() ? args["flight_number"].asString() : "");
    if (name == "search_flights")         return Tools::search_flights(args);
    if (name == "get_all_gates")          return Tools::get_all_gates();
    if (name == "get_gate_by_id")         return Tools::get_gate_by_id(args);
    if (name == "get_gate_by_number")     return Tools::get_gate_by_number(args);
    if (name == "get_available_gates")    return Tools::get_available_gates();
    if (name == "get_runway_status")      return Tools::get_runway_status();
    if (name == "get_latest_weather")     return Tools::get_latest_weather();
    if (name == "resolve_incident")       return Tools::resolve_incident(args.get("id", "").asString());
    if (name == "update_flight_status")   return Tools::update_flight_status(args);
    if (name == "assign_flight_to_gate")  return Tools::assign_flight_to_gate(args);
    if (name == "create_incident")
        return Tools::create_incident(
            args.get("title", "").asString(),
            args.get("description", "").asString(),
            args.get("severity", "").asString(),
            args.get("location", "").asString());

    Json::Value err;
    err["error"] = "Unknown tool: " + name;
    return err;
}
