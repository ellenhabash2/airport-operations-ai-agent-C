#include "database_manager.h"
#include <iostream>
#include <stdexcept>
#include <chrono>

DatabaseManager &DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::initialize(const std::string &connection_string, std::size_t pool_size)
{
    std::lock_guard<std::mutex> lock(mutex_);

    connection_string_ = connection_string;
    pool_size_ = (pool_size > 0) ? pool_size : 1;
    idle_.clear();
    live_count_ = 0;
    initialized_ = true;

    // Open one connection now so startup fails loudly on a bad configuration
    // instead of on the first request.
    try
    {
        auto probe = std::make_unique<pqxx::connection>(connection_string_);
        if (probe->is_open())
        {
            idle_.push_back(std::move(probe));
            live_count_ = 1;
            std::cout << "Database connection established successfully (pool size "
                      << pool_size_ << ")." << std::endl;
        }
        else
        {
            std::cerr << "Failed to establish database connection." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        // Not fatal: the pool reconnects lazily once PostgreSQL is reachable.
        std::cerr << "Database initialization error: " << e.what() << std::endl;
    }
}

std::shared_ptr<pqxx::connection> DatabaseManager::wrap(pqxx::connection *raw)
{
    return std::shared_ptr<pqxx::connection>(
        raw, [this](pqxx::connection *c) { this->release(c); });
}

std::shared_ptr<pqxx::connection> DatabaseManager::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (!initialized_ || connection_string_.empty())
    {
        throw std::runtime_error("Database is not initialized.");
    }

    for (;;)
    {
        // 1. Reuse an idle connection if there is one.
        if (!idle_.empty())
        {
            std::unique_ptr<pqxx::connection> conn = std::move(idle_.front());
            idle_.pop_front();

            bool usable = false;
            try { usable = conn->is_open(); } catch (...) { usable = false; }

            if (usable)
            {
                return wrap(conn.release());
            }

            // Dead connection: drop it and look again.
            --live_count_;
            continue;
        }

        // 2. Room to open a new one.
        if (live_count_ < pool_size_)
        {
            ++live_count_;      // reserve the slot before releasing the lock
            lock.unlock();

            try
            {
                auto conn = std::make_unique<pqxx::connection>(connection_string_);
                if (!conn->is_open())
                {
                    throw std::runtime_error("Database connection is not available.");
                }
                lock.lock();
                return wrap(conn.release());
            }
            catch (...)
            {
                lock.lock();
                --live_count_;
                cv_.notify_one();
                throw;
            }
        }

        // 3. Pool exhausted: wait for a connection to come back.
        if (cv_.wait_for(lock, std::chrono::seconds(5),
                         [this] { return !idle_.empty() || live_count_ < pool_size_; }))
        {
            continue;
        }

        throw std::runtime_error("Timed out waiting for a free database connection.");
    }
}

void DatabaseManager::release(pqxx::connection *raw)
{
    std::unique_ptr<pqxx::connection> conn(raw);

    bool reusable = false;
    try { reusable = conn && conn->is_open(); } catch (...) { reusable = false; }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (reusable)
        {
            idle_.push_back(std::move(conn));   // still counted in live_count_
        }
        else
        {
            --live_count_;                      // closed for good
        }
    }

    cv_.notify_one();
}

bool DatabaseManager::isConnected()
{
    try
    {
        auto conn = getConnection();
        return conn && conn->is_open();
    }
    catch (const std::exception &)
    {
        return false;
    }
}