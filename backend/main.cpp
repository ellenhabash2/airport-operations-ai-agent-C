#include <drogon/drogon.h>
#include <iostream>
#include <cstdlib>
#include "database/database_manager.h"

int main()
{
    try
    {
        // Get database connection string from environment variables
        const char *db_host = std::getenv("DB_HOST");
        const char *db_port = std::getenv("DB_PORT");
        const char *db_name = std::getenv("DB_NAME");
        const char *db_user = std::getenv("DB_USER");
        const char *db_password = std::getenv("DB_PASSWORD");

        db_host = db_host ? db_host : "localhost";
        db_port = db_port ? db_port : "5432";
        db_name = db_name ? db_name : "aeromind";
        db_user = db_user ? db_user : "aeromind_user";
        db_password = db_password ? db_password : "aeromind_password";
        
        // Construct connection string
        std::string connection_string = "postgresql://" + std::string(db_user) + ":" + 
                                       std::string(db_password) + "@" + std::string(db_host) + 
                                       ":" + std::string(db_port) + "/" + std::string(db_name);

        // Initialize database
        DatabaseManager::getInstance().initialize(connection_string);

        // Configure Drogon
        const char *port_value = std::getenv("PORT");
        const uint16_t port = static_cast<uint16_t>(port_value ? std::stoi(port_value) : 8848);

        drogon::app()
            .setLogPath("./logs")
            .setLogLevel(trantor::LogLevel::kInfo)
            .addListener("0.0.0.0", port);

        std::cout << "AeroMind Backend starting on port " << port << "..." << std::endl;
        std::cout << "Database: " << db_name << " @ " << db_host << ":" << db_port << std::endl;

        // Run the server
        drogon::app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
