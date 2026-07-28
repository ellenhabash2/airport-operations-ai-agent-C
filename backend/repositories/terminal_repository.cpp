#include "terminal_repository.h"
#include "database/database_manager.h"
#include <pqxx/pqxx>

namespace
{
Terminal terminalFromRow(const pqxx::row &row)
{
    return {row["id"].as<int>(), row["name"].c_str(), row["code"].c_str(), row["capacity"].as<int>()};
}

Json::Value flightFromRow(const pqxx::row &row)
{
    Json::Value flight;
    flight["id"] = row["id"].as<int>();
    flight["flight_number"] = row["flight_number"].c_str();
    flight["airline"] = row["airline"].c_str();
    flight["airline_name"] = row["airline_name"].c_str();
    flight["aircraft"] = row["aircraft"].c_str();
    flight["gate_id"] = row["gate_id"].as<int>();
    flight["gate"] = row["gate_number"].c_str();
    flight["terminal_id"] = row["terminal_id"].as<int>();
    flight["terminal"] = row["terminal_code"].c_str();
    flight["runway"] = row["runway_code"].is_null() ? "" : row["runway_code"].c_str();
    flight["origin"] = row["origin"].c_str();
    flight["destination"] = row["destination"].c_str();
    flight["status"] = row["status"].c_str();
    flight["departure_time"] = row["departure_time"].c_str();
    flight["arrival_time"] = row["arrival_time"].c_str();
    return flight;
}

constexpr const char *kTerminalColumns = "SELECT id, name, code, capacity FROM terminals ";
}

std::vector<Terminal> TerminalRepository::findAll()
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    const auto rows = txn.exec(std::string(kTerminalColumns) + "ORDER BY code, id");
    std::vector<Terminal> terminals;
    terminals.reserve(rows.size());
    for (const auto &row : rows) terminals.push_back(terminalFromRow(row));
    txn.commit();
    return terminals;
}

std::optional<Terminal> TerminalRepository::findById(int terminalId)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    const auto rows = txn.exec_params(std::string(kTerminalColumns) + "WHERE id = $1", terminalId);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    return terminalFromRow(rows[0]);
}

std::optional<Terminal> TerminalRepository::findByName(const std::string &name)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    const auto rows = txn.exec_params(
        std::string(kTerminalColumns) + "WHERE LOWER(name) = LOWER($1) ORDER BY id LIMIT 1", name);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    return terminalFromRow(rows[0]);
}

Json::Value TerminalRepository::findFlightsByTerminal(int terminalId)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    const auto rows = txn.exec_params(
        "SELECT DISTINCT f.id, f.flight_number, a.name AS airline_name, a.iata_code AS airline, "
        "ac.registration_number AS aircraft, f.gate_id, g.gate_number, g.terminal_id, "
        "t.code AS terminal_code, r.runway_code, f.origin, f.destination, f.status, "
        "f.departure_time, f.arrival_time "
        "FROM flights f "
        "JOIN airlines a ON a.id = f.airline_id "
        "JOIN aircraft ac ON ac.id = f.aircraft_id "
        "JOIN gates g ON g.id = f.gate_id "
        "JOIN terminals t ON t.id = g.terminal_id "
        "LEFT JOIN runways r ON r.id = f.runway_id "
        "WHERE g.terminal_id = $1 "
        "ORDER BY f.departure_time, f.flight_number, f.id", terminalId);
    Json::Value flights(Json::arrayValue);
    for (const auto &row : rows) flights.append(flightFromRow(row));
    txn.commit();
    return flights;
}

std::optional<TerminalStatus> TerminalRepository::getStatus(int terminalId)
{
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    const auto rows = txn.exec_params(
        "SELECT t.id, t.name, t.code, t.capacity, "
        "COUNT(g.id) AS total_gates, "
        "COUNT(g.id) FILTER (WHERE g.status = 'AVAILABLE') AS available_gates, "
        "COUNT(g.id) FILTER (WHERE g.status = 'OCCUPIED') AS occupied_gates, "
        "COUNT(g.id) FILTER (WHERE g.status IN ('MAINTENANCE', 'CLOSED')) AS non_operational_gates, "
        "(SELECT COUNT(*) FROM flights f JOIN gates fg ON fg.id = f.gate_id "
        " WHERE fg.terminal_id = t.id AND f.status IN ('SCHEDULED', 'BOARDING', 'DELAYED')) AS active_flights "
        "FROM terminals t LEFT JOIN gates g ON g.terminal_id = t.id "
        "WHERE t.id = $1 GROUP BY t.id, t.name, t.code, t.capacity", terminalId);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    const auto &row = rows[0];
    TerminalStatus status;
    status.terminal = terminalFromRow(row);
    status.totalGates = row["total_gates"].as<int>();
    status.availableGates = row["available_gates"].as<int>();
    status.occupiedGates = row["occupied_gates"].as<int>();
    status.nonOperationalGates = row["non_operational_gates"].as<int>();
    status.activeFlights = row["active_flights"].as<int>();
    return status;
}

