#ifndef I_SERVER_DISPATCHER_HPP
#define I_SERVER_DISPATCHER_HPP

#include "agent_connection.hpp"
#include "frame_transport.hpp"
#include "protocol/lptf_protocol.hpp"
#include "session_manager.hpp"

class IServerDispatcher {
 public:
  virtual ~IServerDispatcher() = default;

  virtual void handleFrame(FrameTransport& agent, const Frame& frame) = 0;
  virtual void sendCommand(AgentConnection& agent,
                           const CommandPayload& Command) = 0;
  virtual SessionManager& getSessionManager() = 0;
  virtual void onAgentDisconnect(AgentConnection& agent) = 0;
};

#endif
