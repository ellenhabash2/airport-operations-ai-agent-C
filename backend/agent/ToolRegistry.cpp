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
}

Json::Value ToolRegistry::getToolDefinitions()
{
    Json::Value tools(Json::arrayValue);

    // Read tools
    tools.append(makeTool("find_delayed_flights", "Returns all flights that are currently delayed."));
    tools.append(makeTool("get_active_incidents", "Returns airport operational incidents (open, investigating, resolved)."));
    tools.append(makeTool("get_all_flights", "Returns the full list of flights."));
    tools.append(makeToolWithParam("get_flight_details", "Returns full details of a single flight by its numeric id.",
                                   "id", "The numeric flight id, e.g. \"42\"."));
    tools.append(makeTool("get_available_gates", "Returns all gates and their status."));
    tools.append(makeTool("get_runway_status", "Returns all runways and their status."));
    tools.append(makeTool("get_latest_weather", "Returns the latest weather report for the airport."));

    // Action tools
    tools.append(makeToolWithParam("resolve_incident", "Marks an incident as resolved by its numeric id.",
                                   "id", "The numeric incident id, e.g. \"7\"."));

    // create_incident takes several parameters
    {
        Json::Value tool = makeTool("create_incident", "Creates a new airport operational incident.");
        Json::Value &props = tool["function"]["parameters"]["properties"];
        Json::Value s; s["type"] = "string";
        s["description"] = "Short incident title.";        props["title"] = s;
        s["description"] = "Detailed description.";        props["description"] = s;
        s["description"] = "Severity: LOW, MEDIUM, HIGH, or CRITICAL."; props["severity"] = s;
        s["description"] = "Location, e.g. Terminal 1.";   props["location"] = s;
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
    if (name == "get_available_gates")    return Tools::get_available_gates();
    if (name == "get_runway_status")      return Tools::get_runway_status();
    if (name == "get_latest_weather")     return Tools::get_latest_weather();
    if (name == "resolve_incident")       return Tools::resolve_incident(args.get("id", "").asString());
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
