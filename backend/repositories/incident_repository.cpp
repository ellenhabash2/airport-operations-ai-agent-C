#include "incident_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>
#include <algorithm>
#include <cctype>

namespace
{
// incidents.id is an integer column. The agent's resolve_incident tool feeds
// the model's raw argument in here, so a non-numeric value would make
// PostgreSQL throw and fail the whole agent turn. Guard it in one place.
bool isPositiveInteger(const std::string &value)
{
    return !value.empty() &&
           std::all_of(value.begin(), value.end(),
                       [](unsigned char ch) { return std::isdigit(ch); }) &&
           std::any_of(value.begin(), value.end(), [](char ch) { return ch != '0'; });
}

constexpr const char *kIncidentColumns =
    "SELECT id, title, description, severity, location, status, created_at, resolved_at FROM incidents ";

Json::Value incidentRowToJson(const pqxx::row &row)
{
    Json::Value incident;
    incident["id"] = row["id"].c_str();
    incident["title"] = row["title"].c_str();
    incident["description"] = row["description"].c_str();
    incident["severity"] = row["severity"].c_str();
    incident["location"] = row["location"].is_null() ? "" : row["location"].c_str();
    incident["status"] = row["status"].c_str();
    incident["created_at"] = row["created_at"].c_str();
    incident["resolved_at"] = row["resolved_at"].is_null() ? "" : row["resolved_at"].c_str();
    return incident;
}

Json::Value queryIncidents(const std::string &whereClause)
{
    Json::Value incidents(Json::arrayValue);

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec(std::string(kIncidentColumns) + whereClause +
                                    "ORDER BY created_at DESC, id DESC");

        for (auto row : res)
        {
            incidents.append(incidentRowToJson(row));
        }

        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }

    return incidents;
}
}

Json::Value IncidentRepository::getAllIncidents()
{
    return queryIncidents("");
}

Json::Value IncidentRepository::getActiveIncidents()
{
    return queryIncidents("WHERE status <> 'RESOLVED' ");
}

Json::Value IncidentRepository::getIncidentsBySeverity(const std::string &severity)
{
    Json::Value incidents(Json::arrayValue);
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(std::string(kIncidentColumns) +
        "WHERE severity = $1 ORDER BY created_at DESC, id DESC", severity);
    for (const auto &row : rows) incidents.append(incidentRowToJson(row));
    txn.commit();
    return incidents;
}

Json::Value IncidentRepository::searchIncidents(const std::string &query)
{
    std::string escaped;
    escaped.reserve(query.size());
    for (char ch : query)
    {
        if (ch == '%' || ch == '_' || ch == '\\') escaped.push_back('\\');
        escaped.push_back(ch);
    }
    Json::Value incidents(Json::arrayValue);
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(std::string(kIncidentColumns) +
        "WHERE title ILIKE $1 ESCAPE '\\' OR description ILIKE $1 ESCAPE '\\' "
        "OR COALESCE(location, '') ILIKE $1 ESCAPE '\\' "
        "ORDER BY created_at DESC, id DESC", "%" + escaped + "%");
    for (const auto &row : rows) incidents.append(incidentRowToJson(row));
    txn.commit();
    return incidents;
}

Json::Value IncidentRepository::getIncidentById(const std::string &id)
{
    if (!isPositiveInteger(id)) return Json::Value();
    auto conn = DatabaseManager::getInstance().getConnection();
    pqxx::work txn(*conn);
    auto rows = txn.exec_params(std::string(kIncidentColumns) + "WHERE id = $1", id);
    Json::Value incident;
    if (!rows.empty()) incident = incidentRowToJson(rows[0]);
    txn.commit();
    return incident;
}

Json::Value IncidentRepository::createIncident(const std::string &title, const std::string &description,
                                               const std::string &severity, const std::string &location)
{
    Json::Value incident;

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(
            "INSERT INTO incidents (title, description, severity, location, created_at) "
            "VALUES ($1, $2, $3, $4, NOW()) RETURNING id, status, created_at",
            title, description, severity, location
        );

        if (!res.empty())
        {
            incident["id"] = res[0]["id"].c_str();
            incident["title"] = title;
            incident["description"] = description;
            incident["severity"] = severity;
            incident["location"] = location;
            incident["status"] = res[0]["status"].c_str();
            incident["created_at"] = res[0]["created_at"].c_str();
        }

        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database insert error: " << e.what() << std::endl;
        throw;
    }

    return incident;
}

Json::Value IncidentRepository::resolveIncident(const std::string &id)
{
    Json::Value result;

    // Reject non-numeric ids up front so a bad id never reaches PostgreSQL.
    if (!isPositiveInteger(id))
    {
        result["found"] = false;
        return result;
    }

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        // A conditional update makes resolution race-safe: at most one caller
        // can transition a non-resolved incident and receive a row.
        pqxx::result res = txn.exec_params(
            "UPDATE incidents SET status = 'RESOLVED', resolved_at = NOW() "
            "WHERE id = $1 AND status <> 'RESOLVED' "
            "RETURNING id, title, description, severity, location, status, created_at, resolved_at",
            id
        );

        if (res.empty())
        {
            auto existing = txn.exec_params("SELECT 1 FROM incidents WHERE id = $1", id);
            result["found"] = !existing.empty();
            result["already_resolved"] = !existing.empty();
            txn.commit();
            return result;
        }

        txn.commit();

        auto row = res[0];
        result["found"] = true;
        result["already_resolved"] = false;
        result["incident"]["id"] = row["id"].c_str();
        result["incident"]["title"] = row["title"].c_str();
        result["incident"]["description"] = row["description"].c_str();
        result["incident"]["severity"] = row["severity"].c_str();
        result["incident"]["location"] = row["location"].is_null() ? "" : row["location"].c_str();
        result["incident"]["status"] = row["status"].c_str();
        result["incident"]["created_at"] = row["created_at"].c_str();
        result["incident"]["resolved_at"] = row["resolved_at"].is_null() ? "" : row["resolved_at"].c_str();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database update error: " << e.what() << std::endl;
        throw;
    }

    return result;
}
