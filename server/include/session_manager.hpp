#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "agent_connection.hpp"

class SessionManager {
  // TODO add constructor and destructor
 private:
  std::unordered_map<int, AgentConnection> agents_;
  int dashboardFd_ = -1;

  std::unordered_map<std::string, int>
      agentsByTarget_;  // agent id currently hostname, filedescriptor
  std::unordered_map<int, std::string>
      targetByFd_;  // agent id currently hostname, filedescriptor

 public:
  const std::unordered_map<int, AgentConnection>& getAgents() const;
  int getDashboardFd();
  void addAgent(int fileDescriptor, std::unique_ptr<ISocket> incoming);
  void recordAgentTarget(int fd, const std::string& target);
  void deleteAgentTarget(int fd, const std::string& target);
  AgentConnection& getAgent(int fileDescriptor);
  AgentConnection& getAgentByTarget(const std::string& target);
  bool isDashboard() const;
  bool isDashboardConnection(int fd) const;
  void setDashboardFd(int fileDescriptor);
  void resetDashboard();
  //   int getDashboardFd();
  AgentConnection& getDashboard();
  std::string getTargetByFd(int fileDescriptor);
  int getFdByTarget(const std::string& target);
  void removeAgent(int fileDescriptor);
};

#endif