#ifndef AGENT_SESSION_HPP
#define AGENT_SESSION_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/ISocket.hpp"

class AgentSession {
 public:
  AgentSession() = default;
  explicit AgentSession(std::unique_ptr<ISocket> sock)
      : socket(std::move(sock)) {}
  AgentSession(std::nullptr_t) = delete;

  AgentSession(const AgentSession&) = delete;
  AgentSession& operator=(const AgentSession&) = delete;
  AgentSession(AgentSession&&) = default;

  std::optional<Frame> tryExtractFrame();
  bool isValid() const { return socket && socket->isValid(); }

  std::unique_ptr<ISocket> socket;
  std::vector<std::uint8_t> buffer;

  const RegisterPayload& getAgentInfo() const {
    if (!isRegistered_) {
      throw std::runtime_error("Agent not registered");
    }
    return agentInfo_;
  }

  void setAgentInfo(const RegisterPayload& info) {
    agentInfo_ = info;
    isRegistered_ = true;
  }

  bool isRegistered() const { return isRegistered_; }
  void setRegistered(bool registered) { isRegistered_ = registered; }

 private:
  std::optional<LptfHeader> header_;
  void consume(std::size_t n);
  std::vector<std::uint8_t> slice(std::size_t offset, std::size_t len) const;

  RegisterPayload agentInfo_;
  bool isRegistered_ = false;
};

#endif