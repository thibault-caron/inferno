
#ifndef DATABASE_CONNECTION_HPP
#define DATABASE_CONNECTION_HPP

#include <memory>
#include <pqxx/pqxx>
#include <sstream>
#include <string>
#include <string_view>

#include "env_helper.hpp"

class IDatabaseConnection {
 public:  // Low-level query execution — return raw results
  virtual ~IDatabaseConnection() = default;
  virtual pqxx::result execute(const std::string& query) = 0;
  virtual pqxx::result executeParams(const std::string& query,
                                     const pqxx::params& params) = 0;
};

class DatabaseConnection : public IDatabaseConnection {
 private:
  pqxx::connection conn_;

 public:
  static std::string buildConnectionString(std::string_view password,
                                           std::string_view user,
                                           std::string_view dbName,
                                           std::string_view dbHost,
                                           std::string_view dbPort);
  explicit DatabaseConnection();
  explicit DatabaseConnection(std::string_view password, std::string_view user,
                              std::string_view dbName, std::string_view dbHost,
                              std::string_view dbPort);

  // Low-level query execution — return raw results
  pqxx::result execute(const std::string& query) override;
  pqxx::result executeParams(const std::string& query,
                             const pqxx::params& params) override;
};

#endif