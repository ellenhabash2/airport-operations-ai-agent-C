#include <drogon/drogon.h>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <filesystem>
#include "database/database_manager.h"

namespace
{
// Small helper so every env lookup has an explicit default in one place.
std::string envOr(const char *name, const std::string &fallback)
{
    const char *value = std::getenv(name);
    return (value && *value) ? std::string(value) : fallback;
}
}

int main()
{
    try
    {
        const std::string db_host     = envOr("DB_HOST", "localhost");
        const std::string db_port     = envOr("DB_PORT", "5432");
        const std::string db_name     = envOr("DB_NAME", "aeromind");
        const std::string db_user     = envOr("DB_USER", "aeromind_user");
        const std::string db_password = envOr("DB_PASSWORD", "aeromind_password");

        const std::string connection_string =
            "postgresql://" + db_user + ":" + db_password + "@" +
            db_host + ":" + db_port + "/" + db_name;

        DatabaseManager::getInstance().initialize(connection_string);

        if (!DatabaseManager::getInstance().isConnected())
        {
            std::cerr << "WARNING: starting without a database connection. "
                      << "/health will report 503 until PostgreSQL is reachable." << std::endl;
        }

        const uint16_t port = static_cast<uint16_t>(std::stoi(envOr("PORT", "8848")));

        // Drogon throws at startup if the log directory does not exist.
        std::error_code ec;
        std::filesystem::create_directories("./logs", ec);

        // Drogon defaults to a single IO thread. /agent/query performs blocking
        // calls to the LLM provider, so one agent request would freeze the whole
        // server. Give the framework a real thread pool.
        unsigned int threads = std::thread::hardware_concurrency();
        if (threads < 4) threads = 4;

        drogon::app()
            .setLogPath("./logs")
            .setLogLevel(trantor::Logger::kInfo)
            .setThreadNum(threads)
            .addListener("0.0.0.0", port);

        std::cout << "AeroMind Backend starting on port " << port
                  << " with " << threads << " IO threads..." << std::endl;
        std::cout << "Database: " << db_name << " @ " << db_host << ":" << db_port << std::endl;

        drogon::app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}