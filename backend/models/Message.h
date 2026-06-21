#pragma once
#include <string>

struct Message
{
    int id{};
    int conversation_id{};
    std::string role;
    std::string content;
    std::string created_at;
};
