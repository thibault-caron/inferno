#ifndef AGENT_DISPATCHER_HPP
#define AGENT_DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "dispatcher.hpp"
#include "protocol/lptf_protocol.hpp"

class AgentDispatcher : public Dispatcher {
 public:
  // std::string getSenderName() override { return senderName; };
  explicit AgentDispatcher();

  AgentDispatcher(const AgentDispatcher&) = delete;
  ~AgentDispatcher() = default;
  AgentDispatcher& operator=(const AgentDispatcher&) = delete;

  void handleFrame(AgentSession& agent, const Frame& frame) override;

  bool getRegisterWasSent() { return registerWasSent; };
  void sendRegister(AgentSession& session);

 private:
  // const std::string senderName{"agent"};
  bool registerWasSent{false};
  void sendResponse(AgentSession& session, std::uint16_t id,
                    ResponseStatus status, const std::string& data);
  void onCommand(AgentSession& session,
                 const std::vector<std::uint8_t>& payload);
  void onDisconnect(AgentSession& session);
  void onError(const std::vector<std::uint8_t>& payload);
};

#endif