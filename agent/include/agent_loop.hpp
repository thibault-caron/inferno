#ifndef AGENT_LOOP_HPP
#define AGENT_LOOP_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "agent_dispatcher.hpp"
#include "agent_session.hpp"
#include "poller/i_poller.hpp"
#include "logger.hpp"

class AgentLoop {
 public:
  // heartbeatMs — how long to wait for server data before sending HEALTHCHECK
  // retryMs     — how long to wait between reconnection attempts
  AgentLoop(IPoller& poller, AgentDispatcher& dispatcher, std::string host,
            std::uint16_t port, int heartbeatMs, int retryMs);

  AgentLoop(const AgentLoop&) = delete;
  AgentLoop& operator=(const AgentLoop&) = delete;

  void run();

 private:
  // Returns heartbeatMs_ when connected, retryMs_ when not.
  // This is what makes the single loop serve both roles without ::sleep().
  int effectiveTimeout() const;

  // Attempt to connect, register, and add the fd to the poller.
  // Called automatically by run() whenever !connected_.
  void tryReconnect();

  // poll() returned 0 while connected — future HEALTHCHECK send.
  void onTimeout();

  // Fill buffer and drain all complete frames through the dispatcher.
  void onReadable();

  // poll() reported POLLERR/POLLNVAL — treat as disconnection.
  void onError();

  // Single cleanup point: poller.remove → socket.close → connected_ = false.
  void onDisconnect();

  IPoller& poller_;
  AgentDispatcher& dispatcher_;
  std::string host_;
  std::uint16_t port_;
  int heartbeatMs_;
  int retryMs_;

  bool running_;
  bool connected_;

  // Persists across reconnections. resetSession() swaps the socket internally;
  // agentInfo_ (hostname, arch, os) is never cleared.
  AgentSession session_;

  Logger logger_{"agent_loop"};
};

#endif  // AGENT_LOOP_HPP