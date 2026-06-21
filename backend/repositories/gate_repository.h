#pragma once
#include <json/json.h>
#include <string>

class GateRepository
{
public:
    static Json::Value getAllGates();
};
