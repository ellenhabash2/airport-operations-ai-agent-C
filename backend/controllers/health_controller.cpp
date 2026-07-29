#include "health_controller.h"
#include <drogon/HttpAppFramework.h>
#include <json/json.h>
#include "database/database_manager.h"

void HealthController::health(const HttpRequestPtr &, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // A health check that always says "ok" is worse than none: the Docker
    // Compose healthcheck (curl -f /health) and any monitoring would treat the
    // service as healthy even while the database is unreachable. Report the
    // real database state so /health returns 503 until PostgreSQL is up, which
    // is exactly what main.cpp promises at startup.
    bool dbConnected = DatabaseManager::getInstance().isConnected();

    Json::Value response;
    response["status"] = dbConnected ? "ok" : "degraded";
    response["service"] = "AeroMind";
    response["database"] = dbConnected ? "connected" : "unavailable";

    auto http_response = HttpResponse::newHttpJsonResponse(response);
    http_response->setStatusCode(dbConnected ? k200OK : k503ServiceUnavailable);
    callback(http_response);
}
