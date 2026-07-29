#pragma once
#include "ToolExecutionRecord.h"
#include <json/json.h>
#include <string>
#include <vector>

// Safety boundary between internal execution data and the public agent response.
//
// AgentSafety is responsible for everything that leaves the backend in the
// `tool_executions` array: it redacts sensitive argument values, maps internal
// error payloads to safe public categories, and never exposes raw tool results,
// SQL, stack traces, provider headers, or secrets.
class AgentSafety
{
public:
    // Recursive, case-insensitive redaction of sensitive keys. Returns a
    // sanitized COPY; the input is never mutated. Non-sensitive values pass
    // through unchanged.
    static Json::Value sanitizeArguments(const Json::Value &arguments);

    // Maps an internal tool error payload ({"error":..., "code":...}) to one of
    // a small set of safe public categories. Never returns the raw message.
    static std::string safeErrorCode(const Json::Value &result);

    // Builds the public `tool_executions` array from the internal trace:
    //   { tool, status, arguments (sanitized), duration_ms, sequence,
    //     error_code?, call_id? }
    // Raw results are never included.
    static Json::Value toolExecutionsJson(const std::vector<ToolExecutionRecord> &executions);
};
