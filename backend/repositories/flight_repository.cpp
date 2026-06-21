#include "flight_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

Json::Value FlightRepository::getAllFlights()
{
    Json::Value flights(Json::arrayValue);
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec(
            "SELECT f.id, f.flight_number, a.iata_code AS airline, ac.registration_number AS aircraft, "
            "g.gate_number, r.runway_code, f.origin, f.destination, f.status, f.departure_time, f.arrival_time "
            "FROM flights f "
            "JOIN airlines a ON a.id = f.airline_id "
            "JOIN aircraft ac ON ac.id = f.aircraft_id "
            "LEFT JOIN gates g ON g.id = f.gate_id "
            "LEFT JOIN runways r ON r.id = f.runway_id "
            "ORDER BY f.departure_time "
            "LIMIT 150");
        
        int index = 0;
        for (auto row : res)
        {
            Json::Value flight;
            flight["id"] = row["id"].c_str();
            flight["flight_number"] = row["flight_number"].c_str();
            flight["airline"] = row["airline"].c_str();
            flight["aircraft"] = row["aircraft"].c_str();
            flight["gate"] = row["gate_number"].is_null() ? "" : row["gate_number"].c_str();
            flight["runway"] = row["runway_code"].is_null() ? "" : row["runway_code"].c_str();
            flight["origin"] = row["origin"].c_str();
            flight["destination"] = row["destination"].c_str();
            flight["status"] = row["status"].c_str();
            flight["departure_time"] = row["departure_time"].c_str();
            flight["arrival_time"] = row["arrival_time"].c_str();
            
            flights[index++] = flight;
        }
        
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }
    
    return flights;
}

Json::Value FlightRepository::getFlightById(const std::string &id)
{
    Json::Value flight;
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec_params(
            "SELECT f.id, f.flight_number, a.iata_code AS airline, ac.registration_number AS aircraft, "
            "g.gate_number, r.runway_code, f.origin, f.destination, f.status, f.departure_time, f.arrival_time "
            "FROM flights f "
            "JOIN airlines a ON a.id = f.airline_id "
            "JOIN aircraft ac ON ac.id = f.aircraft_id "
            "LEFT JOIN gates g ON g.id = f.gate_id "
            "LEFT JOIN runways r ON r.id = f.runway_id "
            "WHERE f.id = $1",
            id
        );
        
        if (!res.empty())
        {
            auto row = res[0];
            flight["id"] = row["id"].c_str();
            flight["flight_number"] = row["flight_number"].c_str();
            flight["airline"] = row["airline"].c_str();
            flight["aircraft"] = row["aircraft"].c_str();
            flight["gate"] = row["gate_number"].is_null() ? "" : row["gate_number"].c_str();
            flight["runway"] = row["runway_code"].is_null() ? "" : row["runway_code"].c_str();
            flight["origin"] = row["origin"].c_str();
            flight["destination"] = row["destination"].c_str();
            flight["status"] = row["status"].c_str();
            flight["departure_time"] = row["departure_time"].c_str();
            flight["arrival_time"] = row["arrival_time"].c_str();
        }
        
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }
    
    return flight;
}

Json::Value FlightRepository::createFlight(const std::string &flight_number, const std::string &airline_id,
                                           const std::string &aircraft_id, const std::string &origin,
                                           const std::string &destination, const std::string &status)
{
    Json::Value flight;
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec_params(
            "INSERT INTO flights (flight_number, airline_id, aircraft_id, origin, destination, status) "
            "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
            flight_number, airline_id, aircraft_id, origin, destination, status
        );
        
        if (!res.empty())
        {
            flight["id"] = res[0]["id"].c_str();
            flight["flight_number"] = flight_number;
            flight["airline_id"] = airline_id;
            flight["aircraft_id"] = aircraft_id;
            flight["origin"] = origin;
            flight["destination"] = destination;
            flight["status"] = status;
        }
        
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database insert error: " << e.what() << std::endl;
        throw;
    }
    
    return flight;
}
