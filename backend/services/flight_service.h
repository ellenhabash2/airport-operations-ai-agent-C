#pragma once
#include <functional>
#include <json/json.h>
#include <string>

class FlightService {
public:
    struct Dependencies {
        std::function<Json::Value()> all;
        std::function<Json::Value()> delayed;
        std::function<Json::Value(const std::string &)> byId;
    };
    FlightService();
    explicit FlightService(Dependencies dependencies);
    Json::Value getAll() const;
    Json::Value getDelayed() const;
    Json::Value getById(const std::string &id) const;
private:
    Dependencies dependencies_;
};
