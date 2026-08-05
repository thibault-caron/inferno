#ifndef AGENT_DISPATCHER_HPP
#define AGENT_DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "dispatcher/i_agent_dispatcher.hpp"
#include "metrics/metrics_controller.hpp"
#include "protocol/lptf_protocol.hpp"
#include "system_monitor/i_system_monitor.hpp"

class AgentDispatcher : public IAgentDispatcher {
 public:
  explicit AgentDispatcher(ISystemMonitor& monitor);

  AgentDispatcher(const AgentDispatcher&) = delete;
  ~AgentDispatcher() = default;
  AgentDispatcher& operator=(const AgentDispatcher&) = delete;

  void handleFrame(FrameTransport& agent, const Frame& frame) override;

  void sendRegister(AgentSession& session) override;
  void setMetricsController(
      std::shared_ptr<MetricsController> controller) override;

 private:
  ISystemMonitor& monitor_;  // injected -used to read OS info for REGISTER
  std::shared_ptr<MetricsController> metricsController_{nullptr};

  void send(AgentSession& session, std::uint16_t id, ResponseStatus status,
            const std::vector<std::uint8_t>& data,
            CommandType cmdType = CommandType::OS_INFO,
            MessageType type = MessageType::RESPONSE);
  void onCommand(AgentSession& session,
                 const std::vector<std::uint8_t>& payload);
  void onDisconnect(AgentSession& session);
  void onError(const std::vector<std::uint8_t>& payload);

  // Helpers for each command type
  void startMetrics(AgentSession& session, const CommandPayload& command);
  void stopMetrics(AgentSession& session, const CommandPayload& command);
  void osInfo(AgentSession& session, const CommandPayload& command);
  void processesList(AgentSession& session, const CommandPayload& command);
  void shellCommand(AgentSession& session, const CommandPayload& command);
  void sendResponseChunked(AgentSession& session, std::uint16_t id,
                           ResponseStatus status, CommandType cmdType,
                           const std::vector<std::uint8_t>& data);
  std::vector<std::uint8_t> createResponseChunk(
      std::uint16_t id, ResponseStatus status, CommandType cmdType,
      std::size_t chunkIndex, std::size_t totalChunks, std::size_t start,
      std::size_t end, const std::vector<std::uint8_t>& data);
};

#endif