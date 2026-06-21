#include "runway_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

Json::Value RunwayRepository::getAllRunways()
{
    Json::Value runways(Json::arrayValue);
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec("SELECT id, runway_code, status, length_meters, surface FROM runways ORDER BY runway_code");
        
        int index = 0;
        for (auto row : res)
        {
            Json::Value runway;
            runway["id"] = row["id"].c_str();
            runway["runway_code"] = row["runway_code"].c_str();
            runway["status"] = row["status"].c_str();
            runway["length_meters"] = row["length_meters"].as<int>();
            runway["surface"] = row["surface"].c_str();
            
            runways[index++] = runway;
        }
        
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }
    
    return runways;
}
