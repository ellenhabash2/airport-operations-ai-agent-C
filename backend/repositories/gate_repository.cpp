#include "gate_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

Json::Value GateRepository::getAllGates()
{
    Json::Value gates(Json::arrayValue);
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec(
            "SELECT g.id, g.gate_number, g.terminal_id, t.code AS terminal_code, g.status "
            "FROM gates g "
            "JOIN terminals t ON t.id = g.terminal_id "
            "ORDER BY t.code, g.gate_number");
        
        int index = 0;
        for (auto row : res)
        {
            Json::Value gate;
            gate["id"] = row["id"].c_str();
            gate["gate_number"] = row["gate_number"].c_str();
            gate["terminal_id"] = row["terminal_id"].c_str();
            gate["terminal_code"] = row["terminal_code"].c_str();
            gate["status"] = row["status"].c_str();
            
            gates[index++] = gate;
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
