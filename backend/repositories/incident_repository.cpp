#include "incident_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

Json::Value IncidentRepository::getAllIncidents()
{
    Json::Value incidents(Json::arrayValue);

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec("SELECT id, title, description, severity, location, status, created_at FROM incidents ORDER BY created_at DESC LIMIT 50");

        int index = 0;
        for (auto row : res)
        {
            Json::Value incident;
            incident["id"] = row["id"].c_str();
            incident["title"] = row["title"].c_str();
            incident["description"] = row["description"].c_str();
            incident["severity"] = row["severity"].c_str();
            incident["location"] = row["location"].is_null() ? "" : row["location"].c_str();
            incident["status"] = row["status"].c_str();
            incident["created_at"] = row["created_at"].c_str();

            incidents[index++] = incident;
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

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        // Step 1: check the incident exists and read its current status
        pqxx::result existing = txn.exec_params(
            "SELECT status FROM incidents WHERE id = $1", id);

        if (existing.empty())
        {
            result["found"] = false;
            return result;
        }

        std::string currentStatus = existing[0]["status"].c_str();

        // Business rule: an already-resolved incident cannot be resolved again
        if (currentStatus == "RESOLVED")
        {
            result["found"] = true;
            result["already_resolved"] = true;
            return result;
        }

        // Step 2: resolve it and record the resolution time
        pqxx::result res = txn.exec_params(
            "UPDATE incidents SET status = 'RESOLVED', resolved_at = NOW() "
            "WHERE id = $1 "
            "RETURNING id, title, description, severity, location, status, created_at, resolved_at",
            id
        );

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

