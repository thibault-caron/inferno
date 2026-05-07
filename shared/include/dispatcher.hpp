#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <iostream>
#include <string>

#include "agent_session.hpp"
#include "exception/lptf_exception.hpp"
#include "exception/socket_exception.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "socket/socket_helper.hpp"
#include "logger.hpp"

class Dispatcher {
 public:
  //   const std::string senderName{"server"};
  //   Dispatcher(std::string who) : logger_(const std::string& who) {};
  Dispatcher(const std::string& who) : logger_(who) {}
  virtual ~Dispatcher() = default;
  // Must be overriden by children
  //   virtual std::string getSenderName() { return senderName; };
  virtual void handleFrame(AgentSession& agent, const Frame& frame) = 0;

  // ── Incoming message handlers ───────────────────────
  void onError(const std::vector<std::uint8_t>& payload);

  // ── Outgoing message builders ───────────────────────
  void sendError(AgentSession& agent, ErrorType code, const std::string& msg);

  // ── I/O ─────────────────────────────────────────────
  void sendFrame(AgentSession& session, Frame& frame);

 protected:
  Logger logger_;
};

#endif