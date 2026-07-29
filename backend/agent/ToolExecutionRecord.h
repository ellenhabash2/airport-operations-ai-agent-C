#pragma once
#include <cstddef>
#include <json/json.h>
#include <string>

// A single tool invocation captured during one agent turn.
//
// This is the internal, trusted record produced by the AgentLoop. It retains
// the raw structured tool result so the PresentationService can build a
// deterministic presentation from it. The public /agent/query response never
// exposes `arguments` verbatim or `result` at all: AgentSafety produces a
// sanitized public view (see AgentSafety::toolExecutionsJson).
struct ToolExecutionRecord
{
    std::string callId;        // provider tool_call id (may be empty)
    std::string tool;          // canonical tool name
    Json::Value arguments;     // raw arguments as sent to the tool (internal)
    Json::Value result;        // raw structured tool result (internal)
    bool success{true};        // false when the tool returned an error payload
    std::string errorCode;     // safe category when !success (e.g. "not_found")
    long long durationMs{0};   // monotonic elapsed time, milliseconds, >= 0
    std::size_t sequence{0};   // 0-based order of execution within the turn
};
