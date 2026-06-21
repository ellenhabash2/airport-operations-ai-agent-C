#pragma once
#include <json/json.h>
#include <string>

class JsonUtils
{
public:
    static Json::Value errorResponse(const std::string &message);
    static Json::Value successResponse(const Json::Value &data);
};
