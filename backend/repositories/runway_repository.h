#pragma once
#include <json/json.h>
#include <string>

class RunwayRepository
{
public:
    static Json::Value getAllRunways();
    static Json::Value getRunwayById(int id);
    static Json::Value getRunwayByCode(const std::string &runwayCode);
    static bool updateStatus(int id, const std::string &status);
};