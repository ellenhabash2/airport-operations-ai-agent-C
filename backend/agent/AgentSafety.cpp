#include "AgentSafety.h"
#include <algorithm>
#include <cctype>

namespace {

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool endsWith(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Precise sensitive-name matching. We deliberately avoid redacting the bare word
// "key", because operational fields (e.g. a runway code lookup) could legitimately
// use it. Matching is on the full lowercased key name plus a few clear suffixes.
bool isSensitiveKey(const std::string &rawKey)
{
    const std::string key = toLower(rawKey);
    static const std::vector<std::string> exact{
        "authorization", "token", "api_key", "apikey", "x-api-key", "password",
        "passwd", "secret", "jwt", "cookie", "cookies", "credentials", "credential",
        "headers", "bearer", "auth", "access_token", "refresh_token", "session",
        "set-cookie", "private_key", "provider_headers", "database_url",
        "connection_string"};
    if (std::find(exact.begin(), exact.end(), key) != exact.end()) return true;
    if (key.find("password") != std::string::npos) return true;
    return endsWith(key, "_token") || endsWith(key, "_secret") || endsWith(key, "_key") ||
           endsWith(key, "_password") || endsWith(key, "apikey");
}

} // namespace

Json::Value AgentSafety::sanitizeArguments(const Json::Value &arguments)
{
    if (arguments.isObject()) {
        Json::Value sanitized(Json::objectValue);
        for (const auto &key : arguments.getMemberNames()) {
            if (isSensitiveKey(key)) sanitized[key] = "[redacted]";
            else sanitized[key] = sanitizeArguments(arguments[key]);  // recurse into nested
        }
        return sanitized;
    }
    if (arguments.isArray()) {
        Json::Value sanitized(Json::arrayValue);
        for (const auto &item : arguments) sanitized.append(sanitizeArguments(item));
        return sanitized;
    }
    return arguments;  // scalars pass through
}

std::string AgentSafety::safeErrorCode(const Json::Value &result)
{
    if (!result.isObject() || !result.isMember("error")) return "";
    const std::string code = result.get("code", "").asString();
    if (code.empty()) return "internal_error";
    const std::string lower = code;  // domain codes are already lowercase snake_case
    if (lower.find("not_found") != std::string::npos) return "not_found";
    if (lower.find("unavailable") != std::string::npos || lower.find("not_operational") != std::string::npos ||
        lower.find("already_resolved") != std::string::npos || lower.find("conflict") != std::string::npos)
        return "conflict";
    if (lower.find("forbidden") != std::string::npos) return "forbidden";
    if (lower.find("unauthorized") != std::string::npos) return "unauthorized";
    if (lower.find("provider") != std::string::npos) return "provider_error";
    if (lower.find("timeout") != std::string::npos) return "timeout";
    if (lower.rfind("invalid", 0) == 0 || lower.find("validation") != std::string::npos) return "validation_error";
    return "internal_error";  // never expose the raw code/message verbatim
}

Json::Value AgentSafety::toolExecutionsJson(const std::vector<ToolExecutionRecord> &executions)
{
    Json::Value array(Json::arrayValue);
    for (const auto &record : executions) {
        Json::Value item;
        item["tool"] = record.tool;
        item["status"] = record.success ? "success" : "error";
        item["arguments"] = sanitizeArguments(record.arguments);
        item["duration_ms"] = static_cast<Json::Int64>(record.durationMs < 0 ? 0 : record.durationMs);
        item["sequence"] = static_cast<Json::UInt64>(record.sequence);
        if (!record.callId.empty()) item["call_id"] = record.callId;
        if (!record.success) item["error_code"] = safeErrorCode(record.result);
        array.append(item);
    }
    return array;
}
