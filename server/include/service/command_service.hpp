#ifndef COMMAND_SERVICE_HPP
#define COMMAND_SERVICE_HPP

#include <optional>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "repository/command_repository.hpp"
#include "repository/database_connection.hpp"
#include "session_manager.hpp"

class ICommandService {
 public:
  virtual ~ICommandService() = default;

  virtual void save(DashboardCommand& commandDashboard) = 0;
//   virtual std::string getTarget(std::uint32_t commandId) = 0;
//   virtual void deleteTarget(std::uint32_t commandId) = 0;
  virtual std::vector<DashboardCommand> findByAgentId(
      const std::string& agentId, int limit = 50) = 0;
  virtual CommandType getCommandType(
      const std::uint32_t commandId) = 0;
};

// command_repository.hpp
class CommandService : public ICommandService {
 private:
  ICommandRepository& repository_;
  SessionManager& sessionManager_;
  // map<command.id, target string = mac adress>
//   std::map<std::uint32_t, std::string> commandTargets_;

 public:
  explicit CommandService(ICommandRepository& repository,
                          SessionManager& sessionManager)
      : repository_(repository), sessionManager_(sessionManager) {}

  void save(DashboardCommand& commandDashboard) override;
//   std::string getTarget(std::uint32_t commandId) override;
//   void deleteTarget(std::uint32_t commandId) override;
  std::vector<DashboardCommand> findByAgentId(const std::string& agentId,
                                              int limit = 50) override;
  CommandType getCommandType(const std::uint32_t commandId) override;
};

#endif