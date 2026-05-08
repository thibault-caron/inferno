#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <sys/epoll.h>

#include <unordered_map>

#include "agent_session.hpp"
#include "dispatcher/i_dispatcher.hpp"
#include "logger.hpp"
#include "poller/i_poller.hpp"
#include "tcp_server.hpp"

class Reactor {
 public:
  explicit Reactor(TcpServer& server, IDispatcher& dispatcher, IPoller& poller)
      : dispatcher_(dispatcher), server_(server), poller_(poller) {}
  Reactor() = delete;
  Reactor(const Reactor&) = delete;
  Reactor& operator=(const Reactor&) = delete;
  ~Reactor() = default;

  void run();
  void stop() { running_ = false; };

 private:
  IDispatcher& dispatcher_;
  TcpServer& server_;
  IPoller& poller_;
  Logger logger_{"reactor"};

  bool running_ = false;
  std::unordered_map<int, AgentSession> agents_;

  // internal helpers — each maps to one "something happened" situation
  void onNewConnection();
  void onAgentReady(int fileDescriptor);
  void onAgentDisconnected(int fileDescriptor);
};

#endif