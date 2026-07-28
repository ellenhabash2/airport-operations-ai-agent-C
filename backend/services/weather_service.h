#pragma once
#include <functional>
#include <json/json.h>
#include <string>
class WeatherService {
public:
    struct Dependencies {
        std::function<Json::Value()> recent, latest;
        std::function<Json::Value(const std::string &, float, float, float)> create;
    };
    WeatherService(); explicit WeatherService(Dependencies dependencies);
    Json::Value getRecent() const; Json::Value getLatest() const;
    Json::Value create(const std::string &, float, float, float) const;
private: Dependencies dependencies_;
};
