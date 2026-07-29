#include "runway_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

namespace
{
constexpr const char *kRunwayColumns =
    "SELECT id, runway_code, status, length_meters, surface FROM runways ";

Json::Value runwayRowToJson(const pqxx::row &row)
{
    Json::Value runway;
    runway["id"] = row["id"].as<int>();
    runway["runway_code"] = row["runway_code"].c_str();
    runway["status"] = row["status"].c_str();
    runway["length_meters"] = row["length_meters"].as<int>();
    runway["surface"] = row["surface"].c_str();
    return runway;
}

Json::Value queryRunways(const std::string &suffix, const pqxx::params &params = {})
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(std::string(kRunwayColumns) + suffix, params);
    Json::Value runways(Json::arrayValue);
    for (const auto &row : rows) runways.append(runwayRowToJson(row));
    txn.commit();
    return runways;
}
}

Json::Value RunwayRepository::getAllRunways()
{
    return queryRunways("ORDER BY runway_code");
}

Json::Value RunwayRepository::getRunwayById(int id)
{
    Json::Value result;
    pqxx::params params; params.append(id);
    auto runways = queryRunways("WHERE id = $1", params);
    result["found"] = !runways.empty();
    if (!runways.empty()) result["runway"] = runways[0];
    return result;
}

Json::Value RunwayRepository::getRunwayByCode(const std::string &runwayCode)
{
    Json::Value result;
    pqxx::params params; params.append(runwayCode);
    auto runways = queryRunways("WHERE runway_code = $1", params);
    result["found"] = !runways.empty();
    if (!runways.empty()) result["runway"] = runways[0];
    return result;
}

bool RunwayRepository::updateStatus(int id, const std::string &status)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto result = txn.exec_params(
        "UPDATE runways SET status = $2 WHERE id = $1 RETURNING id", id, status);
    txn.commit();
    return !result.empty();
}