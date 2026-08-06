#ifndef AGENT_SESSION_HPP
#define AGENT_SESSION_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "codec/protocol_parser.hpp"
#include "frame_transport.hpp"
#include "protocol/lptf_protocol.hpp"
#include "socket/i_socket.hpp"

enum class RegisterState : std::uint8_t { PENDING, SENT, OK, REJECTED };

class AgentSession : public FrameTransport {
 public:
  explicit AgentSession(const bool encryption = false)
      : FrameTransport(), encryption_(encryption) {};
  explicit AgentSession(std::unique_ptr<ISocket> sock,
                        const bool encryption = false)
      : FrameTransport(std::move(sock)), encryption_(encryption) {}
  AgentSession(std::nullptr_t) = delete;

  AgentSession(const AgentSession&) = delete;
  AgentSession& operator=(const AgentSession&) = delete;
  AgentSession(AgentSession&&) = default;

  void resetSession();
  // TODO : socket and buffer should be private and send() method too ?
  // ===== socket related methods =====
  bool connect(const std::string& host, std::uint16_t port);

  // ===== Register payload related methods =====
  const OsInfoPayload& getAgentInfo() const override;
  void setAgentInfo(const OsInfoPayload& info) override;

  // ===== Register and registration state related method =====
  RegisterState getRegistered_() const { return registered_; };
  void setRegistered_(RegisterState state) { registered_ = state; };
  bool isDisconnectRequested() { return disconnectRequested_; }
  void setDisconnectRequest(bool request) { disconnectRequested_ = request; }

 private:
  RegisterState registered_{RegisterState::REJECTED};
  bool encryption_;
  bool disconnectRequested_ = false;
};

#endif