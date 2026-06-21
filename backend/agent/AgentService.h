#pragma once
#include <json/json.h>
#include <string>

class AgentService
{
public:
    virtual ~AgentService() = default;

    // TODO(ai-phase): Orchestrate the agentic loop once Gemini and tools are enabled.
    virtual Json::Value answerQuery(const std::string &user_query) = 0;
};
