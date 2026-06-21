#pragma once
#include <string>

struct User
{
    int id{};
    std::string username;
    std::string email;
    std::string role;
    std::string created_at;
};
