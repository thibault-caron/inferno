#include "agent_connection.hpp"

#include <sstream>

#include "exception/socket_exception.hpp"

// ===== Register payload related methods =====
const OsInfoPayload& AgentConnection::getAgentInfo() const {
  if (!isRegistered_) {
    // TODO : use custom exception
    throw std::runtime_error("Agent not registered");
  }
  return agentInfo_;
}

void AgentConnection::setAgentInfo(const OsInfoPayload& info) {
  agentInfo_ = info;
  isRegistered_ = true;
}