#pragma once
#include <json/json.h>
#include <string>

class RunwayRepository
{
public:
    static Json::Value getAllRunways();
};
