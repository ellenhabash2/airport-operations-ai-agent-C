#include "AgentLoop.h"
#include <memory>

AgentLoop::Result AgentLoop::run(Json::Value messages, const Json::Value &tools,
                                const Provider &provider, const ToolExecutor &execute,
                                int maxIterations)
{
    Result result;
    for (int step = 0; step < maxIterations; ++step) {
        const Json::Value response = provider(messages, tools);
        if (response.isMember("error")) { result.providerFailed = true; return result; }
        if (!response.isMember("choices") || response["choices"].empty() ||
            !response["choices"][0].isMember("message")) return result;

        const Json::Value message = response["choices"][0]["message"];
        if (!message.isMember("tool_calls") || message["tool_calls"].empty()) {
            if (message.isMember("content") && !message["content"].isNull())
                result.answer = message["content"].asString();
            return result;
        }

        messages.append(message);
        for (const auto &call : message["tool_calls"]) {
            const std::string name = call["function"]["name"].asString();
            result.toolsUsed.append(name);
            Json::Value args(Json::objectValue), toolResult;
            const auto &raw = call["function"]["arguments"];
            bool valid = raw.isObject();
            if (valid) args = raw;
            else if (raw.isString()) {
                Json::CharReaderBuilder builder;
                std::string error;
                auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
                const auto value = raw.asString();
                valid = reader->parse(value.data(), value.data() + value.size(), &args, &error) && args.isObject();
            }
            if (!valid) toolResult["error"] = "Invalid tool arguments";
            else toolResult = execute(name, args);

            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            Json::Value toolMessage;
            toolMessage["role"] = "tool";
            toolMessage["tool_call_id"] = call["id"];
            toolMessage["content"] = Json::writeString(writer, toolResult);
            messages.append(toolMessage);
        }
    }
    result.maxIterationsReached = true;
    return result;
}
