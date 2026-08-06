#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <memory>  // for unique_ptr
#include <string>

#include "agent_loop.hpp"
#include "dispatcher/agent_dispatcher.hpp"
#include "env_helper.hpp"
#include "logger.hpp"
#include "poller/poller.hpp"
#include "system_monitor/i_system_monitor.hpp"
#include "system_monitor/system_monitor_factory.hpp"
int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);

  const std::string host = EnvHelper::resolveString("SERVER_HOST");
  Logger::info("agent main", "SERVER_HOST = " + host);

  const std::uint16_t port = EnvHelper::resolvePort();

  constexpr int kHeartbeatMs = 30'000;  // 30s — HEALTHCHECK cadence
  constexpr int kRetryMs = 10'000;      // 10s  — reconnection cadence

  Poller poller;
  // AgentDispatcher dispatcher;
  std::unique_ptr<ISystemMonitor> monitor =
      SystemMonitorFactory::createSystemMonitor();
  AgentDispatcher dispatcher(*monitor);
  bool encryption = EnvHelper::resolveTlsEnabled();

  AgentLoop loop(poller, dispatcher, host, port, kHeartbeatMs, kRetryMs,
                 encryption);

  loop.run();
  return 0;
}