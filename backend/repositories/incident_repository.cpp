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
