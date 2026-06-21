#pragma once
#include <string>
#include <memory>
#include <pqxx/pqxx>
#include <mutex>

class DatabaseManager
{
public:
    static DatabaseManager &getInstance();
    
    void initialize(const std::string &connection_string);
    std::shared_ptr<pqxx::connection> getConnection();
    bool isConnected() const;

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    std::string connection_string_;
    bool connected_ = false;
    mutable std::mutex mutex_;
};
