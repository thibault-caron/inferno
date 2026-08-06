#ifndef I_AGENT_DISPATCHER_HPP
#define I_AGENT_DISPATCHER_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "metrics/metrics_controller.hpp"
#include "protocol/lptf_protocol.hpp"
#include "system_monitor/i_system_monitor.hpp"

class IAgentDispatcher {
 public:
  virtual ~IAgentDispatcher() = default;

  virtual void handleFrame(FrameTransport& agent, const Frame& frame) = 0;

  virtual void sendRegister(AgentSession& session) = 0;
  virtual void setMetricsController(
      std::shared_ptr<MetricsController> controller) = 0;
};

#endif