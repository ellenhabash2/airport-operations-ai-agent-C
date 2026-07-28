#pragma once
#include <functional>
#include <json/json.h>
class RunwayService {
public:
    using Loader = std::function<Json::Value()>;
    RunwayService(); explicit RunwayService(Loader loader);
    Json::Value getStatus() const;
private: Loader loader_;
};
