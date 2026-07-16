#include "user_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>

Json::Value UserRepository::createUser(const std::string &username, const std::string &email,
                                       const std::string &password_hash)
{
    Json::Value result;

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        // Check if email or username already exists
        pqxx::result existing = txn.exec_params(
            "SELECT id FROM users WHERE email = $1 OR username = $2",
            email, username);

        if (!existing.empty())
        {
            result["error"] = "A user with this email or username already exists";
            return result;
        }

        pqxx::result res = txn.exec_params(
            "INSERT INTO users (username, email, password_hash) "
            "VALUES ($1, $2, $3) "
            "RETURNING id, username, email, role, created_at",
            username, email, password_hash);

        txn.commit();

        auto row = res[0];
        result["id"] = row["id"].c_str();
        result["username"] = row["username"].c_str();
        result["email"] = row["email"].c_str();
        result["role"] = row["role"].c_str();
        result["created_at"] = row["created_at"].c_str();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database error (createUser): " << e.what() << std::endl;
        throw;
    }

    return result;
}

Json::Value UserRepository::findByEmail(const std::string &email)
{
    Json::Value user;

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(
            "SELECT id, username, email, password_hash, role, created_at "
            "FROM users WHERE email = $1",
            email);

        txn.commit();

        if (!res.empty())
        {
            auto row = res[0];
            user["id"] = row["id"].c_str();
            user["username"] = row["username"].c_str();
            user["email"] = row["email"].c_str();
            user["password_hash"] = row["password_hash"].c_str();
            user["role"] = row["role"].c_str();
            user["created_at"] = row["created_at"].c_str();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database error (findByEmail): " << e.what() << std::endl;
        throw;
    }

    return user;
}
