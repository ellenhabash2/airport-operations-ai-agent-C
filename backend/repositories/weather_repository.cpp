#include "weather_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

Json::Value WeatherRepository::getLatestWeather()
{
    Json::Value weather(Json::arrayValue);
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec("SELECT id, condition, visibility_km, wind_speed_kmh, wind_direction, temperature_c, pressure_hpa, created_at FROM weather_reports ORDER BY created_at DESC LIMIT 10");
        
        int index = 0;
        for (auto row : res)
        {
            Json::Value report;
            report["id"] = row["id"].c_str();
            report["condition"] = row["condition"].c_str();
            report["visibility_km"] = row["visibility_km"].as<double>();
            report["wind_speed_kmh"] = row["wind_speed_kmh"].as<double>();
            report["wind_direction"] = row["wind_direction"].c_str();
            report["temperature_c"] = row["temperature_c"].as<double>();
            report["pressure_hpa"] = row["pressure_hpa"].as<double>();
            report["created_at"] = row["created_at"].c_str();
            
            weather[index++] = report;
        }
        
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }
    
    return weather;
}

Json::Value WeatherRepository::createWeather(const std::string &condition, float visibility_km,
                                             float wind_speed_kmh, float temperature_c)
{
    Json::Value weather;
    
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);
        
        pqxx::result res = txn.exec_params(
            "INSERT INTO weather_reports (condition, visibility_km, wind_speed_kmh, temperature_c, created_at) "
            "VALUES ($1, $2, $3, $4, NOW()) RETURNING id, wind_direction, pressure_hpa, created_at",
            condition, visibility_km, wind_speed_kmh, temperature_c
        );
        
        if (!res.empty())
        {
            weather["id"] = res[0]["id"].c_str();
            weather["condition"] = condition;
            weather["visibility_km"] = visibility_km;
            weather["wind_speed_kmh"] = wind_speed_kmh;
            weather["wind_direction"] = res[0]["wind_direction"].c_str();
            weather["temperature_c"] = temperature_c;
            weather["pressure_hpa"] = res[0]["pressure_hpa"].as<double>();
            weather["created_at"] = res[0]["created_at"].c_str();
        }
        
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database insert error: " << e.what() << std::endl;
        throw;
    }
    
    return weather;
}
