#pragma once
#include <functional>
#include <json/json.h>

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
    };

    static Result run(Json::Value messages, const Json::Value &tools,
                      const Provider &provider, const ToolExecutor &execute,
                      int maxIterations = 5);
};
