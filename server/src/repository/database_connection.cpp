#include "repository/database_connection.hpp"

#include <sstream>
#include <stdexcept>
#include <iostream>

#include "env_helper.hpp"

std::string DatabaseConnection::buildConnectionString(std::string_view password,
                                                      std::string_view user,
                                                      std::string_view dbName,
                                                      std::string_view dbHost,
                                                      std::string_view dbPort) {
  std::ostringstream connStr;
  connStr << "dbname=" << dbName << " user=" << user << " password=" << password
          << " host=" << dbHost << " port=" << dbPort;
  return connStr.str();  // ← return the string
}

DatabaseConnection::DatabaseConnection()
    : DatabaseConnection(EnvHelper::resolveString("POSTGRES_PASSWORD"),
                         EnvHelper::resolveString("POSTGRES_USER"),
                         EnvHelper::resolveString("POSTGRES_DB"), "inferno-db",
                         EnvHelper::resolveString("DB_PORT")) {}

DatabaseConnection::DatabaseConnection(std::string_view password,
                                       std::string_view user,
                                       std::string_view dbName,
                                       std::string_view dbHost,
                                       std::string_view dbPort)
    : conn_(buildConnectionString(password, user, dbName, dbHost, dbPort)) {
  try {
    if (!conn_.is_open()) {
      throw std::runtime_error("Failed to open database connection");
    }
  } catch (const pqxx::broken_connection& e) {
    throw std::runtime_error(std::string("Database connection failed: ") +
                             e.what());
  } catch (const pqxx::sql_error& e) {
    throw std::runtime_error(std::string("SQL error during connection: ") +
                             e.what());
  }
}

pqxx::result DatabaseConnection::execute(const std::string& query) {
  try {
    pqxx::work txn(conn_);
    auto result = txn.exec(query);
    txn.commit();
    return result;
  } catch (const pqxx::sql_error& e) {
    throw std::runtime_error(std::string("SQL error: ") + e.what());
  } catch (const pqxx::broken_connection& e) {
    throw std::runtime_error(std::string("Database connection lost: ") +
                             e.what());
  }
}

pqxx::result DatabaseConnection::executeParams(const std::string& query,
                                               const pqxx::params& params) {
  try {
    pqxx::work txn(conn_);
    auto result = txn.exec(query, params); // from txn.exec_params to txn.exec
    txn.commit();
    return result;
  } catch (const pqxx::sql_error& e) {
    throw std::runtime_error(std::string("SQL error: ") + e.what());
  } catch (const pqxx::broken_connection& e) {
    throw std::runtime_error(std::string("Database connection lost: ") +
                             e.what());
  }
}