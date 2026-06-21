#pragma once
#include <string>
#include <vector>

class ToolRegistry
{
public:
    virtual ~ToolRegistry() = default;

    // TODO(ai-phase): Register airport operations tools for agent function calling.
    virtual std::vector<std::string> listTools() const = 0;
};
