#include "ToolRegistry.h"
#include "services/domain_error.h"
#include "tools/tools.h"

#include <stdexcept>

namespace
{
Json::Value objectSchema()
{
    Json::Value schema;
    schema["type"] = "object";
    schema["properties"] = Json::Value(Json::objectValue);
    schema["required"] = Json::Value(Json::arrayValue);
    return schema;
}

Json::Value property(const char *type, const char *description)
{
    Json::Value value;
    value["type"] = type;
    value["description"] = description;
    return value;
}

Json::Value flightStatusProperty()
{
    auto value = property("string", "Supported flight status");
    for (const char *status : {"scheduled", "boarding", "in_flight", "delayed", "cancelled", "landed"})
        value["enum"].append(status);
    return value;
}

Json::Value runwayStatusProperty()
{
    auto value = property("string", "Supported runway status");
    for (const char *status : {"operational", "maintenance", "closed"})
        value["enum"].append(status);
    return value;
}

Json::Value severityProperty()
{
    auto value = property("string", "Incident severity");
    for (const char *severity : {"LOW", "MEDIUM", "HIGH", "CRITICAL"})
        value["enum"].append(severity);
    return value;
}

Json::Value errorResult(const std::string &code, const std::string &message)
{
    Json::Value result;
    result["success"] = false;
    result["error"]["code"] = code;
    result["error"]["message"] = message;
    // Preserve the old top-level fields for saved prompts and clients.
    result["code"] = code;
    result["error_message"] = message;
    return result;
}

bool enumContains(const Json::Value &values, const Json::Value &candidate)
{
    for (const auto &value : values)
        if (value == candidate) return true;
    return false;
}

std::optional<Json::Value> validate(const ToolDefinition &definition, const Json::Value &args)
{
    if (!args.isObject()) return errorResult("invalid_arguments", "Tool arguments must be an object.");
    const auto &schema = definition.parameters;
    for (const auto &required : schema["required"]) {
        const auto name = required.asString();
        if (!args.isMember(name) || args[name].isNull())
            return errorResult("invalid_arguments", "Missing required argument: " + name + ".");
    }
    for (const auto &name : schema["properties"].getMemberNames()) {
        if (!args.isMember(name)) continue;
        const auto &value = args[name];
        const auto &spec = schema["properties"][name];
        const auto type = spec["type"].asString();
        if ((type == "string" && !value.isString()) ||
            (type == "integer" && !value.isInt()))
            return errorResult("invalid_arguments", name + " has the wrong JSON type.");
        if (type == "string" && value.asString().find_first_not_of(" \t\r\n") == std::string::npos)
            return errorResult("invalid_arguments", name + " must not be blank.");
        if (type == "string" && value.asString().size() > 500)
            return errorResult("invalid_arguments", name + " is too long.");
        if (type == "integer" && value.asInt() <= 0)
            return errorResult("invalid_arguments", name + " must be a positive integer.");
        if (spec.isMember("enum") && !enumContains(spec["enum"], value))
            return errorResult("invalid_arguments", name + " has an unsupported value.");
    }
    if (definition.name == "assign_flight_to_gate") {
        const bool hasFlight = args.isMember("flight_id") || args.isMember("flight_number");
        const bool hasGate = args.isMember("gate_id") || args.isMember("gate_number");
        if (!hasFlight || !hasGate)
            return errorResult("invalid_arguments", "A flight identifier and a gate identifier are required.");
    }
    if (definition.name == "update_runway_status" &&
        !args.isMember("runway_id") && !args.isMember("runway_code"))
        return errorResult("invalid_arguments", "A runway_id or runway_code is required.");
    return std::nullopt;
}

ToolDefinition definition(std::string name, std::string description, ToolAccess access,
                          ToolHandler handler, Json::Value schema = objectSchema(), bool deprecated = false)
{
    return {std::move(name), std::move(description), std::move(schema), access,
            std::move(handler), deprecated};
}

Json::Value requiredSchema(const char *name, Json::Value spec)
{
    auto schema = objectSchema();
    schema["properties"][name] = std::move(spec);
    schema["required"].append(name);
    return schema;
}

ToolRegistry buildOperationalRegistry()
{
    ToolRegistry registry;
    auto add = [&](ToolDefinition item) { registry.registerTool(std::move(item)); };
    const auto read = ToolAccess::ReadOnly;
    const auto write = ToolAccess::Write;

    add(definition("find_delayed_flights", "Returns flights currently marked as delayed; use this for broad delayed-flight requests without other filters.", read,
        [](const auto &, const auto &) { return Tools::find_delayed_flights(); }));
    add(definition("get_active_incidents", "Returns only airport incidents that are currently open or under investigation.", read,
        [](const auto &, const auto &) { return Tools::get_active_incidents(); }));
    add(definition("get_all_incidents", "Returns all airport incidents, including resolved incidents.", read,
        [](const auto &, const auto &) { return Tools::get_all_incidents(); }));
    add(definition("get_incidents_by_severity", "Returns incidents matching one severity, including resolved incidents.", read,
        [](const auto &args, const auto &) { return Tools::get_incidents_by_severity(args); },
        requiredSchema("severity", severityProperty())));
    add(definition("search_incidents", "Searches incident titles, descriptions, and locations using case-insensitive partial text matching.", read,
        [](const auto &args, const auto &) { return Tools::search_incidents(args); },
        requiredSchema("query", property("string", "Literal text to search for"))));

    add(definition("get_all_flights", "Returns all flights currently stored in the airport operations database.", read,
        [](const auto &, const auto &) { return Tools::get_all_flights(); }));
    add(definition("get_flight_details", "Deprecated compatibility alias that returns one flight by numeric id; prefer get_flight_by_id.", read,
        [](const auto &args, const auto &) { return Tools::get_flight_details(args["id"].asString()); },
        requiredSchema("id", property("string", "Positive numeric flight id")), true));
    add(definition("get_flight_by_id", "Returns one flight using its internal numeric identifier.", read,
        [](const auto &args, const auto &) { return Tools::get_flight_by_id(std::to_string(args["flight_id"].asInt())); },
        requiredSchema("flight_id", property("integer", "Internal numeric flight identifier"))));
    add(definition("get_flight_by_number", "Returns one flight using its public flight number, such as SB2101.", read,
        [](const auto &args, const auto &) { return Tools::get_flight_by_number(args["flight_number"].asString()); },
        requiredSchema("flight_number", property("string", "Public flight number, for example SB2101"))));
    {
        auto schema = objectSchema();
        auto &props = schema["properties"];
        props["origin"] = property("string", "Partial origin airport code or name");
        props["destination"] = property("string", "Partial destination airport code or name");
        props["status"] = flightStatusProperty();
        props["airline"] = property("string", "Partial airline name");
        props["terminal_id"] = property("integer", "Numeric terminal identifier");
        add(definition("search_flights", "Searches flights using optional origin, destination, status, airline, and terminal filters combined with AND.", read,
            [](const auto &args, const auto &) { return Tools::search_flights(args); }, schema));
    }

    add(definition("get_all_gates", "Returns every gate with its terminal and operational status.", read,
        [](const auto &, const auto &) { return Tools::get_all_gates(); }));
    add(definition("get_available_gates", "Returns only gates currently marked AVAILABLE.", read,
        [](const auto &, const auto &) { return Tools::get_available_gates(); }));
    add(definition("get_gate_by_id", "Returns one gate using its internal numeric identifier.", read,
        [](const auto &args, const auto &) { return Tools::get_gate_by_id(args); },
        requiredSchema("gate_id", property("integer", "Internal numeric gate identifier"))));
    add(definition("get_gate_by_number", "Returns one gate using its public number, such as A03.", read,
        [](const auto &args, const auto &) { return Tools::get_gate_by_number(args); },
        requiredSchema("gate_number", property("string", "Public gate number, for example A03"))));
    add(definition("get_terminal_status", "Returns gate availability and active-flight counts for one terminal.", read,
        [](const auto &args, const auto &) { return Tools::get_terminal_status(args); },
        requiredSchema("terminal_id", property("integer", "Internal numeric terminal identifier"))));
    add(definition("get_flights_by_terminal", "Returns flights currently assigned to gates in one terminal.", read,
        [](const auto &args, const auto &) { return Tools::get_flights_by_terminal(args); },
        requiredSchema("terminal_id", property("integer", "Internal numeric terminal identifier"))));

    add(definition("get_runway_status", "Returns the current status of all runways.", read,
        [](const auto &, const auto &) { return Tools::get_runway_status(); }));
    add(definition("get_runway_by_id", "Returns one runway using its internal numeric identifier.", read,
        [](const auto &args, const auto &) { return Tools::get_runway_by_id(args); },
        requiredSchema("runway_id", property("integer", "Internal numeric runway identifier"))));
    add(definition("get_runway_by_code", "Returns one runway using its public code, such as 08L/26R.", read,
        [](const auto &args, const auto &) { return Tools::get_runway_by_code(args); },
        requiredSchema("runway_code", property("string", "Public runway code, for example 08L/26R"))));
    add(definition("get_latest_weather", "Returns the newest stored weather report for the airport.", read,
        [](const auto &, const auto &) { return Tools::get_latest_weather(); }));

    {
        auto schema = objectSchema();
        schema["properties"]["flight_id"] = property("integer", "Internal numeric flight identifier");
        schema["properties"]["status"] = flightStatusProperty();
        schema["required"].append("flight_id"); schema["required"].append("status");
        add(definition("update_flight_status", "Updates one flight to a supported operational status.", write,
            [](const auto &args, const auto &) { return Tools::update_flight_status(args); }, schema));
    }
    {
        auto schema = objectSchema();
        auto &props = schema["properties"];
        props["flight_id"] = property("integer", "Internal flight identifier");
        props["flight_number"] = property("string", "Public flight number");
        props["gate_id"] = property("integer", "Internal gate identifier");
        props["gate_number"] = property("string", "Public gate number, for example A03");
        add(definition("assign_flight_to_gate", "Transactionally assigns a flight to a gate using internal ids or public numbers.", write,
            [](const auto &args, const auto &) { return Tools::assign_flight_to_gate(args); }, schema));
    }
    {
        auto schema = objectSchema();
        schema["properties"]["runway_code"] = property("string", "Public runway code");
        schema["properties"]["runway_id"] = property("integer", "Internal numeric runway identifier");
        schema["properties"]["status"] = runwayStatusProperty();
        schema["required"].append("status");
        add(definition("update_runway_status", "Updates a runway status and returns flights affected by the change.", write,
            [](const auto &args, const auto &) { return Tools::update_runway_status(args); }, schema));
    }
    {
        auto schema = objectSchema();
        auto &props = schema["properties"];
        props["title"] = property("string", "Short incident title");
        props["description"] = property("string", "Detailed incident description");
        props["severity"] = severityProperty();
        props["location"] = property("string", "Optional airport location");
        schema["required"].append("title"); schema["required"].append("description"); schema["required"].append("severity");
        add(definition("create_incident", "Creates a new airport operational incident using the authenticated caller as its actor.", write,
            [](const auto &args, const auto &) { return Tools::create_incident(args["title"].asString(), args["description"].asString(), args["severity"].asString(), args.get("location", "").asString()); }, schema));
    }
    add(definition("resolve_incident", "Marks one incident as resolved using its numeric id.", write,
        [](const auto &args, const auto &) { return Tools::resolve_incident(std::to_string(args["id"].asInt())); },
        requiredSchema("id", property("integer", "Internal numeric incident identifier"))));
    return registry;
}
}

std::string toolAccessName(ToolAccess access)
{
    return access == ToolAccess::Write ? "write" : "read_only";
}

void ToolRegistry::registerTool(ToolDefinition definition)
{
    if (definition.name.empty() || definition.description.empty() || !definition.handler)
        throw std::invalid_argument("Tool definitions require a name, description, and handler");
    if (!definition.parameters.isObject() || definition.parameters["type"] != "object")
        throw std::invalid_argument("Tool parameter schema must be an object schema");
    if (!tools_.emplace(definition.name, std::move(definition)).second)
        throw std::invalid_argument("Duplicate tool name");
}

const ToolDefinition *ToolRegistry::findTool(const std::string &name) const
{
    const auto found = tools_.find(name);
    return found == tools_.end() ? nullptr : &found->second;
}

std::vector<ToolDefinition> ToolRegistry::listTools() const
{
    std::vector<ToolDefinition> result;
    result.reserve(tools_.size());
    for (const auto &[_, definition] : tools_) result.push_back(definition);
    return result;
}

Json::Value ToolRegistry::providerToolSchemas() const
{
    Json::Value result(Json::arrayValue);
    for (const auto &[_, definition] : tools_) {
        Json::Value tool;
        tool["type"] = "function";
        tool["function"]["name"] = definition.name;
        tool["function"]["description"] = definition.description;
        tool["function"]["parameters"] = definition.parameters;
        result.append(tool);
    }
    return result;
}

Json::Value ToolRegistry::execute(const std::string &name, const Json::Value &args,
                                  const ToolExecutionContext &context) const
{
    const auto definition = findTool(name);
    if (!definition) return errorResult("unknown_tool", "Unknown tool: " + name + ".");
    if (definition->access == ToolAccess::Write && (!context.authenticated || !context.userId || context.userId->empty()))
        return errorResult("unauthenticated", "Authentication is required to execute this write tool.");
    if (const auto error = validate(*definition, args)) return *error;
    try {
        return definition->handler(args, context);
    } catch (const DomainError &error) {
        return errorResult(error.code(), error.what());
    } catch (const std::exception &) {
        return errorResult("internal_error", "Tool execution failed safely.");
    }
}

const ToolRegistry &ToolRegistry::operational()
{
    static const ToolRegistry registry = buildOperationalRegistry();
    return registry;
}

Json::Value ToolRegistry::getToolDefinitions() { return operational().providerToolSchemas(); }

Json::Value ToolRegistry::executeTool(const std::string &name, const Json::Value &args,
                                      const ToolExecutionContext &context)
{
    return operational().execute(name, args, context);
}
