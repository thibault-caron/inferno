#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <sys/epoll.h>

#include <unordered_map>

#include "agent_session.hpp"
#include "server_dispatcher.hpp"
#include "tcp_server.hpp"
#include "poller/i_poller.hpp"

class Reactor {
 public:
  explicit Reactor(TcpServer& server, ServerDispatcher& dispatcher, IPoller& poller)
      : dispatcher_(dispatcher), server_(server), poller_(poller) {}
  explicit Reactor() = delete;
  Reactor(const Reactor&) = delete;
  Reactor& operator=(const Reactor&) = delete;
  ~Reactor() = default;

  void run();
  void stop();

 private:
  ServerDispatcher& dispatcher_;
  TcpServer& server_;
  IPoller& poller_;

  
  bool running_ = false;
  std::unordered_map<int, AgentSession> agents_;

  // internal helpers — each maps to one "something happened" situation
  void onNewConnection();
  void onAgentReady(int fileDescriptor);
  void onAgentDisconnected(int fileDescriptor);
};

#endif