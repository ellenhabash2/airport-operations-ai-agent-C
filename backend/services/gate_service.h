#pragma once
#include <functional>
#include <json/json.h>
class GateService {
public:
    struct Dependencies { std::function<Json::Value()> all, available; };
    GateService(); explicit GateService(Dependencies dependencies);
    Json::Value getAll() const; Json::Value getAvailable() const;
private: Dependencies dependencies_;
};
