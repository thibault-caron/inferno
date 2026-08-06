#ifndef COMMAND_REPOSITORY_HPP
#define COMMAND_REPOSITORY_HPP

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "repository/database_connection.hpp"

class ICommandRepository {
 public:
  virtual ~ICommandRepository() = default;

  virtual std::uint32_t save(const DashboardCommand& cmd) = 0;

  // Latest N commands for one agent (dashboard history panel).
  virtual std::vector<DashboardCommand> findByAgentId(
      const std::string& agentId, int limit = 50) = 0;
  virtual CommandType getCommandType(
      const std::uint32_t commandId) = 0;
};

// command_repository.hpp
class CommandRepository : public ICommandRepository {
 private:
  IDatabaseConnection& db_;  // same connection, different instance

  DashboardCommand rowToDashboardCommand(const pqxx::row& row);

 public:
  explicit CommandRepository(IDatabaseConnection& db) : db_(db) {}

  std::uint32_t save(const DashboardCommand& cmd) override;

  // TODO findAll too?
  std::vector<DashboardCommand> findByAgentId(const std::string& agentId,
                                              int limit = 50) override;
  CommandType getCommandType(const std::uint32_t commandId) override;
};

#endif