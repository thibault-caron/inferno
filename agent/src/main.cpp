#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <string>

#include "agent_dispatcher.hpp"
#include "agent_loop.hpp"
#include "env_helper.hpp"
#include "poller/poller.hpp"

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);

  const std::string host = EnvHelper::resolveServerHost();
  const std::uint16_t port = EnvHelper::resolveServerPort();

  constexpr int kHeartbeatMs = 30'000;  // 30s — HEALTHCHECK cadence
  constexpr int kRetryMs = 5'000;       // 5s  — reconnection cadence

  Poller poller;
  AgentDispatcher dispatcher;
  AgentLoop loop(poller, dispatcher, host, port, kHeartbeatMs, kRetryMs);

  loop.run();
  return 0;
}