#include "flight_repository.h"
#include "database/database_manager.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <pqxx/pqxx>

namespace
{
bool isPositiveInteger(const std::string &value)
{
    return !value.empty() && value != "0" &&
           std::all_of(value.begin(), value.end(),
                       [](unsigned char ch) { return std::isdigit(ch); });
}

constexpr const char *kFlightColumns =
    "SELECT f.id, f.flight_number, a.name AS airline_name, a.iata_code AS airline, "
    "ac.registration_number AS aircraft, f.gate_id, g.gate_number, g.terminal_id, "
    "t.code AS terminal_code, r.runway_code, f.origin, f.destination, f.status, "
    "f.departure_time, f.arrival_time "
    "FROM flights f "
    "JOIN airlines a ON a.id = f.airline_id "
    "JOIN aircraft ac ON ac.id = f.aircraft_id "
    "LEFT JOIN gates g ON g.id = f.gate_id "
    "LEFT JOIN terminals t ON t.id = g.terminal_id "
    "LEFT JOIN runways r ON r.id = f.runway_id ";

Json::Value flightRowToJson(const pqxx::row &row)
{
    Json::Value flight;
    flight["id"] = row["id"].as<int>();
    flight["flight_number"] = row["flight_number"].c_str();
    flight["airline"] = row["airline"].c_str();
    flight["airline_name"] = row["airline_name"].c_str();
    flight["aircraft"] = row["aircraft"].c_str();
    flight["gate_id"] = row["gate_id"].is_null() ? Json::Value() : Json::Value(row["gate_id"].as<int>());
    flight["gate"] = row["gate_number"].is_null() ? "" : row["gate_number"].c_str();
    flight["terminal_id"] = row["terminal_id"].is_null() ? Json::Value() : Json::Value(row["terminal_id"].as<int>());
    flight["terminal"] = row["terminal_code"].is_null() ? "" : row["terminal_code"].c_str();
    flight["runway"] = row["runway_code"].is_null() ? "" : row["runway_code"].c_str();
    flight["origin"] = row["origin"].c_str();
    flight["destination"] = row["destination"].c_str();
    flight["status"] = row["status"].c_str();
    flight["departure_time"] = row["departure_time"].c_str();
    flight["arrival_time"] = row["arrival_time"].c_str();
    return flight;
}

Json::Value gateRowToJson(const pqxx::row &row)
{
    Json::Value gate;
    gate["id"] = row["id"].as<int>();
    gate["gate_number"] = row["gate_number"].c_str();
    gate["terminal_id"] = row["terminal_id"].as<int>();
    gate["terminal_code"] = row["terminal_code"].c_str();
    gate["status"] = row["status"].c_str();
    return gate;
}

Json::Value queryFlights(const std::string &suffix, const pqxx::params &params = {})
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(std::string(kFlightColumns) + suffix, params);
    Json::Value flights(Json::arrayValue);
    for (const auto &row : rows) flights.append(flightRowToJson(row));
    txn.commit();
    return flights;
}
}

Json::Value FlightRepository::getAllFlights()
{
    return queryFlights("ORDER BY f.departure_time, f.flight_number LIMIT 150");
}

Json::Value FlightRepository::getDelayedFlights()
{
    return queryFlights("WHERE f.status = 'DELAYED' ORDER BY f.departure_time, f.flight_number LIMIT 150");
}

Json::Value FlightRepository::getFlightById(const std::string &id)
{
    Json::Value result;
    if (!isPositiveInteger(id)) { result["found"] = false; return result; }
    pqxx::params params; params.append(id);
    auto flights = queryFlights("WHERE f.id = $1", params);
    result["found"] = !flights.empty();
    if (!flights.empty()) result["flight"] = flights[0];
    return result;
}

Json::Value FlightRepository::getFlightByNumber(const std::string &flightNumber)
{
    pqxx::params params; params.append(flightNumber);
    auto flights = queryFlights("WHERE LOWER(f.flight_number) = LOWER($1) ORDER BY f.departure_time LIMIT 1", params);
    Json::Value result; result["found"] = !flights.empty();
    if (!flights.empty()) result["flight"] = flights[0];
    return result;
}

Json::Value FlightRepository::searchFlights(const FlightSearchCriteria &criteria)
{
    std::string sql = "WHERE TRUE ";
    pqxx::params params;
    auto addText = [&](const char *column, const std::string &value) {
        params.append("%" + value + "%");
        sql += "AND " + std::string(column) + " ILIKE $" + std::to_string(params.size()) + " ";
    };
    if (criteria.origin) addText("f.origin", *criteria.origin);
    if (criteria.destination) addText("f.destination", *criteria.destination);
    if (criteria.status) { params.append(*criteria.status); sql += "AND f.status = $" + std::to_string(params.size()) + " "; }
    if (criteria.airline) addText("a.name", *criteria.airline);
    if (criteria.terminalId) { params.append(*criteria.terminalId); sql += "AND g.terminal_id = $" + std::to_string(params.size()) + " "; }
    sql += "ORDER BY f.departure_time, f.flight_number LIMIT 150";
    return queryFlights(sql, params);
}

bool FlightRepository::updateStatus(int flightId, const std::string &status)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto result = txn.exec_params("UPDATE flights SET status = $2 WHERE id = $1 RETURNING id", flightId, status);
    txn.commit();
    return !result.empty();
}

Json::Value FlightRepository::assignGateTransactional(int flightId, int gateId)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    Json::Value result;

    auto flights = txn.exec_params("SELECT id, gate_id FROM flights WHERE id = $1 FOR UPDATE", flightId);
    if (flights.empty()) { result["outcome"] = "flight_not_found"; return result; }

    auto targets = txn.exec_params(
        "SELECT g.id, g.gate_number, g.terminal_id, t.code AS terminal_code, g.status "
        "FROM gates g JOIN terminals t ON t.id = g.terminal_id WHERE g.id = $1 FOR UPDATE OF g", gateId);
    if (targets.empty()) { result["outcome"] = "gate_not_found"; return result; }

    const bool hasPrevious = !flights[0]["gate_id"].is_null();
    const int previousId = hasPrevious ? flights[0]["gate_id"].as<int>() : 0;
    if (previousId == gateId) {
        result["outcome"] = "success";
        result["new_gate"] = gateRowToJson(targets[0]);
        result["previous_gate"] = gateRowToJson(targets[0]);
        txn.commit();
        return result;
    }

    const std::string targetStatus = targets[0]["status"].c_str();
    if (targetStatus != "AVAILABLE") {
        result["outcome"] = (targetStatus == "MAINTENANCE" || targetStatus == "CLOSED")
            ? "gate_not_operational" : "gate_unavailable";
        return result;
    }

    Json::Value previous;
    if (hasPrevious) {
        auto previousRows = txn.exec_params(
            "SELECT g.id, g.gate_number, g.terminal_id, t.code AS terminal_code, g.status "
            "FROM gates g JOIN terminals t ON t.id = g.terminal_id WHERE g.id = $1 FOR UPDATE OF g", previousId);
        if (!previousRows.empty()) previous = gateRowToJson(previousRows[0]);
    }

    txn.exec_params("UPDATE flights SET gate_id = $2 WHERE id = $1", flightId, gateId);
    txn.exec_params("UPDATE gates SET status = 'OCCUPIED' WHERE id = $1", gateId);
    if (hasPrevious) txn.exec_params("UPDATE gates SET status = 'AVAILABLE' WHERE id = $1", previousId);

    result["outcome"] = "success";
    result["previous_gate"] = previous;
    result["new_gate"] = gateRowToJson(targets[0]);
    result["new_gate"]["status"] = "OCCUPIED";
    txn.commit();
    return result;
}
