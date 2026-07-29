#pragma once
#include "ToolExecutionRecord.h"
#include <functional>
#include <json/json.h>
#include <vector>

class AgentLoop
{
public:
    using Provider = std::function<Json::Value(const Json::Value &, const Json::Value &)>;
    using ToolExecutor = std::function<Json::Value(const std::string &, const Json::Value &)>;

    struct Result {
        std::string answer;
        Json::Value toolsUsed{Json::arrayValue};
        bool providerFailed{false};
        bool maxIterationsReached{false};
        Json::Value providerError;
        Json::Value generatedMessages{Json::arrayValue};
        // Phase 8: trusted structured trace of every tool execution this turn,
        // in execution order, with monotonic durations and raw results retained
        // for deterministic presentation generation. Never serialized directly.
        std::vector<ToolExecutionRecord> toolExecutions;
    };

    static Result run(Json::Value messages, const Json::Value &tools,
                      const Provider &provider, const ToolExecutor &execute,
                      int maxIterations = 5);
};
