#pragma once
#include <functional>
#include <json/json.h>
#include <string>
class IncidentService {
public:
    struct Dependencies {
        std::function<Json::Value()> all, active;
        std::function<Json::Value(const std::string &, const std::string &, const std::string &, const std::string &)> create;
        std::function<Json::Value(const std::string &)> resolve;
    };
    IncidentService(); explicit IncidentService(Dependencies dependencies);
    Json::Value getAll() const; Json::Value getActive() const;
    Json::Value create(const std::string &, const std::string &, const std::string &, const std::string &) const;
    Json::Value resolve(const std::string &) const;
private: Dependencies dependencies_;
};
