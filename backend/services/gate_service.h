#pragma once
#include <functional>
#include <json/json.h>
#include <string>

class GateService {
public:
    struct Dependencies {
        std::function<Json::Value()> all, available;
        std::function<Json::Value(const std::string &)> byNumber;
    };
    GateService(); explicit GateService(Dependencies dependencies);
    Json::Value getAll() const; Json::Value getAvailable() const;
    Json::Value getByNumber(const std::string &gateNumber) const;
private: Dependencies dependencies_;
};
