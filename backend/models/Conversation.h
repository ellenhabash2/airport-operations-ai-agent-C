#pragma once
#include <string>

struct Conversation
{
    int id{};
    int user_id{};
    std::string title;
    std::string created_at;
};
