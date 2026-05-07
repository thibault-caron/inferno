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
  std::string getsenderName_() override { return senderName_; };
  explicit AgentDispatcher();

  AgentDispatcher(const AgentDispatcher&) = delete;
  ~AgentDispatcher() = default;
  AgentDispatcher& operator=(const AgentDispatcher&) = delete;

  void handleFrame(AgentSession& agent, const Frame& frame) override;
  void sendRegister(AgentSession& session);
  void sendResponse(AgentSession& session, std::uint16_t id,
                    ResponseStatus status, const std::string& data);

  bool getregisterWasSent_() { return registerWasSent_; };

 private:
  const std::string senderName_{"agent"};
  bool registerWasSent_{false};
};

#endif