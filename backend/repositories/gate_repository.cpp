#include "gate_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

namespace
{
constexpr const char *kGateColumns =
    "SELECT g.id, g.gate_number, g.terminal_id, t.code AS terminal_code, g.status "
    "FROM gates g "
    "JOIN terminals t ON t.id = g.terminal_id ";

Json::Value gateRowToJson(const pqxx::row &row)
{
    Json::Value gate;
    gate["id"] = row["id"].c_str();
    gate["gate_number"] = row["gate_number"].c_str();
    gate["terminal_id"] = row["terminal_id"].c_str();
    gate["terminal_code"] = row["terminal_code"].c_str();
    gate["status"] = row["status"].c_str();
    return gate;
}

Json::Value queryGates(const std::string &whereClause)
{
    Json::Value gates(Json::arrayValue);

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec(std::string(kGateColumns) + whereClause +
                                    "ORDER BY t.code, g.gate_number");

        for (auto row : res)
        {
            gates.append(gateRowToJson(row));
        }

        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }

    return gates;
}
}

Json::Value GateRepository::getAllGates()
{
    return queryGates("");
}

Json::Value GateRepository::getAvailableGates()
{
    return queryGates("WHERE g.status = 'AVAILABLE' ");
}

Json::Value GateRepository::getGateByNumber(const std::string &gateNumber)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(std::string(kGateColumns) +
        "WHERE REGEXP_REPLACE(LOWER(g.gate_number), '^([a-z]+)0+', '\\1') = "
        "REGEXP_REPLACE(LOWER($1), '^([a-z]+)0+', '\\1') ORDER BY g.id LIMIT 1", gateNumber);
    txn.commit();
    Json::Value result; result["found"] = !rows.empty();
    if (!rows.empty()) result["gate"] = gateRowToJson(rows[0]);
    return result;
}
