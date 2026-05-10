#ifndef AGENT_SESSION_HPP
#define AGENT_SESSION_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/i_socket.hpp"

enum class RegisterState : std::uint8_t { PENDING, SENT, OK, REJECTED };

class AgentSession {
 public:
  AgentSession() = default;
  explicit AgentSession(std::unique_ptr<ISocket> sock)
      : socket_(std::move(sock)) {}
  AgentSession(std::nullptr_t) = delete;

  AgentSession(const AgentSession&) = delete;
  AgentSession& operator=(const AgentSession&) = delete;
  AgentSession(AgentSession&&) = default;

  std::optional<Frame> tryExtractFrame();
  void resetSession();
  // TODO : socket and buffer should be private ?
  // ===== socket related methods =====
  bool isValid() const;
  void close();
  bool connect(const std::string& host, std::uint16_t port);
  int getFd() const;
  SocketResult send(const std::vector<std::uint8_t>& bytes);

  // ===== Register payload related methods =====
  const RegisterPayload& getAgentInfo() const;
  void setAgentInfo(const RegisterPayload& info);

  // ===== Register and registration state related method =====
  RegisterState getRegistered_() const { return registered_; };
  void setRegistered_(RegisterState state) { registered_ = state; };
  bool getIsRegistered() const { return isRegistered_; }

  // ===== Buffer related methods =====
  SocketResult receiveIntoBuffer();
  void appendToBuffer(const std::vector<std::uint8_t>& bytes);
  std::size_t bufferSize() const { return buffer_.size(); }

 private:
  std::unique_ptr<ISocket> socket_;
  std::vector<std::uint8_t> buffer_;
  std::optional<LptfHeader> header_;
  RegisterState registered_{RegisterState::REJECTED};

  // ===== Buffer related private methods =====
  void consume(std::size_t n);
  std::vector<std::uint8_t> slice(std::size_t offset, std::size_t len) const;

  RegisterPayload agentInfo_;
  bool isRegistered_ = false;
};

#endif