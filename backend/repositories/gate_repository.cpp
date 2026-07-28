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

Json::Value queryGates(const std::string &whereClause, const pqxx::params &params = {})
{
    Json::Value gates(Json::arrayValue);

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(std::string(kGateColumns) + whereClause, params);

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

Json::Value GateRepository::findAll()
{
    return queryGates("ORDER BY t.code, g.gate_number, g.id");
}

Json::Value GateRepository::findAvailable()
{
    return queryGates("WHERE g.status = 'AVAILABLE' ORDER BY t.code, g.gate_number, g.id");
}

Json::Value GateRepository::findById(int gateId)
{
    pqxx::params params; params.append(gateId);
    auto gates = queryGates("WHERE g.id = $1", params);
    Json::Value result; result["found"] = !gates.empty();
    if (!gates.empty()) result["gate"] = gates[0];
    return result;
}

Json::Value GateRepository::findByNumber(const std::string &gateNumber)
{
    pqxx::params params; params.append(gateNumber);
    auto gates = queryGates(
        "WHERE REGEXP_REPLACE(LOWER(g.gate_number), '^([a-z]+)0+', '\\1') = "
        "REGEXP_REPLACE(LOWER($1), '^([a-z]+)0+', '\\1') "
        "ORDER BY t.code, g.id LIMIT 1", params);
    Json::Value result; result["found"] = !gates.empty();
    if (!gates.empty()) result["gate"] = gates[0];
    return result;
}

Json::Value GateRepository::findByTerminal(int terminalId)
{
    pqxx::params params; params.append(terminalId);
    return queryGates("WHERE g.terminal_id = $1 ORDER BY g.gate_number, g.id", params);
}

bool GateRepository::updateAvailability(int gateId, bool available)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(
        "UPDATE gates SET status = $2 WHERE id = $1 RETURNING id",
        gateId, available ? "AVAILABLE" : "OCCUPIED");
    txn.commit();
    return !rows.empty();
}
