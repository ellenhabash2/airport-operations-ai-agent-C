#pragma once
#include <string>
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <pqxx/pqxx>

// Small connection pool around libpqxx.
//
// getConnection() hands out a shared_ptr whose deleter returns the connection
// to the pool instead of closing it, so every existing call site keeps working
// unchanged:
//
//     auto conn = DatabaseManager::getInstance().getConnection();
//     pqxx::work txn(*conn);
//
// The connection goes back to the pool when `conn` leaves scope.
class DatabaseManager
{
public:
    static DatabaseManager &getInstance();

    // Stores the connection string and opens one connection to verify it works.
    void initialize(const std::string &connection_string, std::size_t pool_size = 10);

    // Borrows a connection from the pool. Blocks (up to 5s) if the pool is
    // exhausted, then throws. Throws if the database is unreachable.
    std::shared_ptr<pqxx::connection> getConnection();

    // Real check: borrows a connection and verifies it is open.
    bool isConnected();

    std::size_t poolSize() const { return pool_size_; }

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    // Called by the shared_ptr deleter.
    void release(pqxx::connection *raw);
    std::shared_ptr<pqxx::connection> wrap(pqxx::connection *raw);

    std::string connection_string_;
    std::size_t pool_size_ = 10;

    std::deque<std::unique_ptr<pqxx::connection>> idle_;
    std::size_t live_count_ = 0;   // idle + currently borrowed
    bool initialized_ = false;

    std::mutex mutex_;
    std::condition_variable cv_;
};