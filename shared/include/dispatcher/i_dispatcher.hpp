#ifndef I_DISPATCHER_HPP
#define I_DISPATCHER_HPP

// #include <iostream>
#include <string>

#include "agent_session.hpp"

class IDispatcher {
 public:
  virtual ~IDispatcher() = default;

  // Must be overriden by children
  virtual void handleFrame(AgentSession& agent, const Frame& frame) = 0;


//   // ── Outgoing message builders ───────────────────────
//   virtual void sendError(AgentSession& agent, ErrorType code,
//                          const std::string& msg) = 0;
                         // ── Incoming message handlers ───────────────────────
  virtual void onError(const std::vector<std::uint8_t>& payload) = 0;

  // ── Outgoing message builders ───────────────────────
  virtual void sendError(AgentSession& agent, ErrorType code, const std::string& msg) = 0;

  // ── I/O ─────────────────────────────────────────────
  virtual void sendFrame(AgentSession& session, Frame& frame) = 0;

};

#endif