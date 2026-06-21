#include "json_utils.h"

Json::Value JsonUtils::errorResponse(const std::string &message)
{
    Json::Value response;
    response["error"] = message;
    return response;
}

Json::Value JsonUtils::successResponse(const Json::Value &data)
{
    Json::Value response;
    response["status"] = "success";
    response["data"] = data;
    return response;
}
