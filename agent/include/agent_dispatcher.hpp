#ifndef SERVER_DISPATCHER_HPP
#define SERVER_DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Idispatcher.hpp"
#include "agent_session.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/ISocket.hpp"
#include "socket/socket_helper.hpp"

class AgentDispatcher : public IDispatcher {
 public:
  std::string getSenderName() override { return senderName; };
  explicit AgentDispatcher();

  AgentDispatcher(const AgentDispatcher&) = delete;
  ~AgentDispatcher() = default;
  AgentDispatcher& operator=(const AgentDispatcher&) = delete;

  void handleFrame(AgentSession& agent, const Frame& frame) override;
  void sendRegister(AgentSession& session);
  void sendResponse(AgentSession& session, std::uint16_t id,
                    ResponseStatus status, const std::string& data);

  bool getRegisterWasSent() { return registerWasSent; };

 private:
  const std::string senderName{"agent"};
  bool registerWasSent{false};
};

#endif