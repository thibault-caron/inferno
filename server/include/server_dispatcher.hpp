#ifndef SERVER_DISPATCHER_HPP
#define SERVER_DISPATCHER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "dispatcher.hpp"
#include "protocol/lptf_protocol.hpp"

class ServerDispatcher : public Dispatcher {
 public:
  explicit ServerDispatcher();
  ServerDispatcher(const ServerDispatcher&) = delete;
  ~ServerDispatcher() = default;
  ServerDispatcher& operator=(const ServerDispatcher&) = delete;

  void handleFrame(AgentSession& agent, const Frame& frame) override;

  std::uint16_t nextId();

 private:
  // ── Incoming message handlers ───────────────────────
  void onRegister(AgentSession& agent,
                  const std::vector<std::uint8_t>& payload);
  void onResponse(AgentSession& agent,
                  const std::vector<std::uint8_t>& payload);
  void onData(const std::vector<std::uint8_t>& payload);

  void sendCommand(AgentSession& agent, CommandType type,
                   const std::string& data = "");
  void sendDisconnect(AgentSession& agent);
  // const std::string senderName{"server"};
  std::uint16_t nextCmdId = 0;
};

#endif