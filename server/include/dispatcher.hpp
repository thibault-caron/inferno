#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/ISocket.hpp"

class Dispatcher {
 public:
  // explicit Dispatcher(std::unique_ptr<ISocket>& socket);
  explicit Dispatcher();
  Dispatcher(const Dispatcher&) = delete;
  ~Dispatcher() = default;
  Dispatcher& operator=(const Dispatcher&) = delete;

  void dispatch(AgentSession& agent, const Frame& frame);
  // ── Incoming message handlers ───────────────────────
  void onRegister(AgentSession& agent,
                  const std::vector<std::uint8_t>& payload);
  void onResponse(AgentSession& agent,
                  const std::vector<std::uint8_t>& payload);
  void onData(const std::vector<std::uint8_t>& payload);
  void onError(const std::vector<std::uint8_t>& payload);

  // ── Outgoing message builders ───────────────────────
  void sendCommand(AgentSession& agent, CommandType type,
                   const std::string& data = "");
  void sendError(AgentSession& agent, ErrorType code, const std::string& msg);
  void sendDisconnect(AgentSession& agent);

  // ── I/O ─────────────────────────────────────────────
  void sendRaw(AgentSession& agent, MessageType type,
               const std::vector<std::uint8_t>& payload = {});

  std::uint16_t nextId();

 private:
  // ISocket& socket_;
  std::uint16_t nextCmdId = 0;
};

#endif