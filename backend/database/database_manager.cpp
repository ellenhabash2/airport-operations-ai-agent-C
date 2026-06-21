#include "database_manager.h"
#include <iostream>

DatabaseManager &DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::initialize(const std::string &connection_string)
{
    try
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_string_ = connection_string;
        auto connection = std::make_shared<pqxx::connection>(connection_string_);
        connected_ = connection->is_open();
        
        if (connected_)
        {
            std::cout << "Database connection established successfully." << std::endl;
        }
        else
        {
            std::cerr << "Failed to establish database connection." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database initialization error: " << e.what() << std::endl;
        connected_ = false;
    }
}

std::shared_ptr<pqxx::connection> DatabaseManager::getConnection()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || connection_string_.empty())
    {
        throw std::runtime_error("Database connection is not available.");
    }
    auto connection = std::make_shared<pqxx::connection>(connection_string_);
    if (!connection->is_open())
    {
        throw std::runtime_error("Database connection is not available.");
    }
    return connection;
}

bool DatabaseManager::isConnected() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}
