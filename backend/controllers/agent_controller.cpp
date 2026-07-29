#include "agent_controller.h"
#include "security/JwtService.h"
#include "services/agent_service.h"
#include "services/conversation_service.h"
#include "services/domain_error.h"
#include <iostream>
#include <optional>

namespace {
HttpResponsePtr errorResponse(const std::string &message, HttpStatusCode status) {
    Json::Value body; body["error"] = message;
    auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(status); return response;
}
std::string bearer(const HttpRequestPtr &request) {
    const auto header = request->getHeader("Authorization");
    return header.rfind("Bearer ", 0) == 0 ? header.substr(7) : "";
}
std::string conversationId(const Json::Value &json) {
    if (!json.isMember("conversation_id") || json["conversation_id"].isNull()) return "";
    const auto &value = json["conversation_id"];
    return value.isString() || value.isIntegral() ? value.asString() : "";
}
HttpStatusCode statusFor(const DomainError &error) {
    if (error.kind() == DomainErrorKind::Forbidden) return k403Forbidden;
    if (error.kind() == DomainErrorKind::ProviderUnavailable) return k502BadGateway;
    if (error.kind() == DomainErrorKind::NotFound) return k404NotFound;
    if (error.kind() == DomainErrorKind::Conflict) return k409Conflict;
    return k400BadRequest;
}
}

void AgentController::queryAgent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        const auto userId = JwtService::getUserId(bearer(req));
        if (userId.empty()) { callback(errorResponse("Invalid or expired token", k401Unauthorized)); return; }
        const auto json = req->getJsonObject();
        if (!json || !json->isMember("query") || !(*json)["query"].isString()) {
            callback(errorResponse("Missing required field: query", k400BadRequest)); return;
        }
        const auto query = (*json)["query"].asString();
        if (query.find_first_not_of(" \t\r\n") == std::string::npos) {
            callback(errorResponse("Field 'query' must not be empty", k400BadRequest)); return;
        }
        std::optional<std::string> requested;
        if (json->isMember("conversation_id") && !(*json)["conversation_id"].isNull()) {
            auto id = conversationId(*json);
            if (id.empty()) { callback(errorResponse("Field 'conversation_id' must be a valid integer", k400BadRequest)); return; }
            requested = id;
        }
        const auto result = AgentService{}.query(userId, query, requested);
        Json::Value body; body["status"] = "success";
        body["conversation_id"] = static_cast<Json::Int64>(std::stoll(result.conversationId));
        body["query"] = query; body["answer"] = result.answer; body["tools_used"] = result.toolsUsed;
        body["tool_executions"] = result.toolExecutions;
        body["presentation"] = result.presentation;
        auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); callback(response);
    } catch (const DomainError &error) { callback(errorResponse(error.what(), statusFor(error))); }
      catch (const std::exception &error) { std::cerr << "Agent request error: " << error.what() << std::endl; callback(errorResponse("Internal server error", k500InternalServerError)); }
}

void AgentController::getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    try {
        const auto userId = JwtService::getUserId(bearer(req));
        if (userId.empty()) { callback(errorResponse("Invalid or expired token", k401Unauthorized)); return; }
        Json::Value body; body["status"] = "success"; body["conversations"] = ConversationService{}.list(userId);
        auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); callback(response);
    } catch (const std::exception &error) { std::cerr << "History request error: " << error.what() << std::endl; callback(errorResponse("Internal server error", k500InternalServerError)); }
}

void AgentController::getConversationMessages(const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, const std::string &id) {
    try {
        const auto userId = JwtService::getUserId(bearer(req));
        if (userId.empty()) { callback(errorResponse("Invalid or expired token", k401Unauthorized)); return; }
        Json::Value body; body["status"] = "success"; body["conversation_id"] = id;
        body["messages"] = ConversationService{}.loadOwnedMessages(id, userId);
        auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); callback(response);
    } catch (const DomainError &error) { callback(errorResponse(error.what(), statusFor(error))); }
      catch (const std::exception &error) { std::cerr << "Conversation messages request error: " << error.what() << std::endl; callback(errorResponse("Internal server error", k500InternalServerError)); }
}

void AgentController::deleteConversation(const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, const std::string &id) {
    try {
        const auto userId = JwtService::getUserId(bearer(req));
        if (userId.empty()) { callback(errorResponse("Invalid or expired token", k401Unauthorized)); return; }
        ConversationService{}.deleteConversation(id, userId);
        Json::Value body; body["status"] = "success"; body["deleted"] = true; body["conversation_id"] = id;
        auto response = HttpResponse::newHttpJsonResponse(body); response->setStatusCode(k200OK); callback(response);
    } catch (const DomainError &error) { callback(errorResponse(error.what(), statusFor(error))); }
      catch (const std::exception &error) { std::cerr << "Conversation delete error: " << error.what() << std::endl; callback(errorResponse("Internal server error", k500InternalServerError)); }
}
