#include "flight_repository.h"
#include "database/database_manager.h"
#include <iostream>
#include <pqxx/pqxx>
#include <algorithm>
#include <cctype>

namespace
{
// The agent's get_flight_details tool passes whatever id string the model
// produces straight through to here. flights.id is an integer column, so a
// non-numeric value ("abc", "42.5", an empty string) makes PostgreSQL throw
// "invalid input syntax for integer" and would turn a single bad tool call
// into a 500 for the whole /agent/query request. Guard it once, in one place,
// so both the REST endpoint and the agent tool stay safe.
bool isPositiveInteger(const std::string &value)
{
    return !value.empty() &&
           std::all_of(value.begin(), value.end(),
                       [](unsigned char ch) { return std::isdigit(ch); });
}
// Every flight query selects the same columns, so map a row in one place.
constexpr const char *kFlightColumns =
    "SELECT f.id, f.flight_number, a.iata_code AS airline, ac.registration_number AS aircraft, "
    "g.gate_number, r.runway_code, f.origin, f.destination, f.status, f.departure_time, f.arrival_time "
    "FROM flights f "
    "JOIN airlines a ON a.id = f.airline_id "
    "JOIN aircraft ac ON ac.id = f.aircraft_id "
    "LEFT JOIN gates g ON g.id = f.gate_id "
    "LEFT JOIN runways r ON r.id = f.runway_id ";

Json::Value flightRowToJson(const pqxx::row &row)
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
    return flight;
}
}

Json::Value FlightRepository::getAllFlights()
{
    Json::Value flights(Json::arrayValue);

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec(std::string(kFlightColumns) +
                                    "ORDER BY f.departure_time LIMIT 150");

        for (auto row : res)
        {
            flights.append(flightRowToJson(row));
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

Json::Value FlightRepository::getDelayedFlights()
{
    Json::Value flights(Json::arrayValue);

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec(std::string(kFlightColumns) +
                                    "WHERE f.status = 'DELAYED' "
                                    "ORDER BY f.departure_time LIMIT 150");

        for (auto row : res)
        {
            flights.append(flightRowToJson(row));
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
    Json::Value result;

    // Reject non-numeric ids up front so a bad id never reaches PostgreSQL.
    if (!isPositiveInteger(id))
    {
        result["found"] = false;
        result["message"] = "Flight id must be a positive integer.";
        return result;
    }

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(std::string(kFlightColumns) + "WHERE f.id = $1", id);
        txn.commit();

        if (res.empty())
        {
            result["found"] = false;
            result["message"] = "No flight exists with id " + id + ".";
            return result;
        }

        result["found"] = true;
        result["flight"] = flightRowToJson(res[0]);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database query error: " << e.what() << std::endl;
        throw;
    }

    return result;
}

Json::Value FlightRepository::createFlight(const std::string &flight_number, const std::string &airline_id,
                                           const std::string &aircraft_id, const std::string &origin,
                                           const std::string &destination,
                                           const std::string &departure_time, const std::string &arrival_time,
                                           const std::string &status)
{
    Json::Value flight;

    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(
            "INSERT INTO flights (flight_number, airline_id, aircraft_id, origin, destination, "
            "departure_time, arrival_time, status) "
            "VALUES ($1, $2, $3, $4, $5, $6::timestamp, $7::timestamp, $8) "
            "RETURNING id, departure_time, arrival_time, status",
            flight_number, airline_id, aircraft_id, origin, destination,
            departure_time, arrival_time, status);

        if (!res.empty())
        {
            flight["id"] = res[0]["id"].c_str();
            flight["flight_number"] = flight_number;
            flight["airline_id"] = airline_id;
            flight["aircraft_id"] = aircraft_id;
            flight["origin"] = origin;
            flight["destination"] = destination;
            flight["departure_time"] = res[0]["departure_time"].c_str();
            flight["arrival_time"] = res[0]["arrival_time"].c_str();
            flight["status"] = res[0]["status"].c_str();
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
