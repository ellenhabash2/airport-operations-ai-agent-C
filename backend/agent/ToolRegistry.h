#pragma once

#include <functional>
#include <json/json.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

enum class ToolAccess { ReadOnly, Write };

struct ToolExecutionContext
{
    bool authenticated{false};
    std::optional<std::string> userId;
    std::string conversationId;
};

using ToolHandler = std::function<Json::Value(const Json::Value &, const ToolExecutionContext &)>;

struct ToolDefinition
{
    std::string name;
    std::string description;
    Json::Value parameters{Json::objectValue};
    ToolAccess access{ToolAccess::ReadOnly};
    ToolHandler handler;
    bool deprecated{false};
};

// Single source of truth for provider schemas, access metadata, and dispatch.
class ToolRegistry
{
public:
    ToolRegistry() = default;

    void registerTool(ToolDefinition definition);
    const ToolDefinition *findTool(const std::string &name) const;
    std::vector<ToolDefinition> listTools() const;
    Json::Value providerToolSchemas() const;
    Json::Value execute(const std::string &name, const Json::Value &args,
                        const ToolExecutionContext &context = {}) const;

    // Compatibility facade used by the existing AgentService and tests.
    static const ToolRegistry &operational();
    static Json::Value getToolDefinitions();
    static Json::Value executeTool(const std::string &name, const Json::Value &args,
                                   const ToolExecutionContext &context = {});

private:
    std::map<std::string, ToolDefinition> tools_;
};

std::string toolAccessName(ToolAccess access);
