#pragma once
#include <json/json.h>
#include <string>

// Bridges the agent's AI model with the concrete Tools.
class ToolRegistry
{
public:
    // Returns the tool definitions in OpenAI "tools" format,
    // so the model knows which tools exist and how to call them.
    static Json::Value getToolDefinitions();

    // Executes a tool by name with the given JSON arguments,
    // and returns the tool's result as JSON.
    static Json::Value executeTool(const std::string &name, const Json::Value &args);
};
