#pragma once

#include <functional>
#include <json/json.h>
#include <optional>
#include <string>

enum class RunwayStatus { Operational, Maintenance, Closed };
std::optional<RunwayStatus> parseRunwayStatus(const std::string &value);
std::string toString(RunwayStatus status);

class RunwayService
{
public:
    struct Dependencies {
        std::function<Json::Value()> all;
        std::function<Json::Value(int)> byId;
        std::function<Json::Value(const std::string &)> byCode;
        std::function<bool(int, const std::string &)> update;
        std::function<Json::Value(int)> affectedFlights;
    };

    RunwayService();
    explicit RunwayService(Dependencies dependencies);
    // Convenience constructor kept so existing status-only tests still build.
    explicit RunwayService(std::function<Json::Value()> all);

    Json::Value getStatus() const;
    Json::Value getById(const std::string &id) const;
    Json::Value getByCode(const std::string &runwayCode) const;
    Json::Value updateStatus(const std::string &id, const std::string &status) const;
    Json::Value updateStatusByCode(const std::string &runwayCode, const std::string &status) const;

private:
    Json::Value applyStatusUpdate(const Json::Value &runway, const std::string &status) const;
    Dependencies dependencies_;
};