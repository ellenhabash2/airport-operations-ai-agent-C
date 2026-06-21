#pragma once
#include <string>

struct Gate
{
    int id{};
    std::string gate_number;
    int terminal_id{};
    std::string status;
};
