#ifndef AGENT_SERVICE_HPP
#define AGENT_SERVICE_HPP

#include <optional>
#include <string>
#include <vector>

#include "agent_connection.hpp"
#include "protocol/lptf_protocol.hpp"
#include "repository/agent_repository.hpp"
#include "repository/database_connection.hpp"
#include "session_manager.hpp"

class IAgentService {
 public:
  virtual ~IAgentService() = default;

  virtual void registerAgent(AgentConnection& agent,
                             const OsInfoPayload& agentInfo,
                             RegisterPayload& registerToSent) = 0;
  virtual void registerDashboard(AgentConnection& dashboard,
                                 const OsInfoPayload& dashboardInfo) = 0;
  virtual std::vector<RegisterPayload> getAllAgents(
      AgentConnection& dashboard) = 0;
  virtual std::optional<std::string> getLastSeen(const std::string& id) = 0;
  virtual void setLastSeen(const std::string& id) = 0;
};

// agent_repository.hpp
class AgentService : public IAgentService {
 private:
  IAgentRepository& repository_;
  SessionManager& sessionManager_;

 public:
  //   explicit AgentService(IDatabaseConnection& db, SessionManager&
  //   sessionManager)
  //       : repository_(db), sessionManager_(sessionManager) {}

  explicit AgentService(IAgentRepository& repository,
                        SessionManager& sessionManager)
      : repository_(repository), sessionManager_(sessionManager) {}

  void registerAgent(AgentConnection& agent, const OsInfoPayload& agentInfo,
                     RegisterPayload& registerToSent);
  void registerDashboard(AgentConnection& dashboard,
                         const OsInfoPayload& dashboardInfo);
  std::vector<RegisterPayload> getAllAgents(
      AgentConnection& dashboard) override;
  std::optional<std::string> getLastSeen(const std::string& id) override;
  void setLastSeen(const std::string& id) override;
};

#endif