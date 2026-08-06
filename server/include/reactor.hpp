#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <sys/epoll.h>

#include <unordered_map>

// #include "agent_session.hpp"
#include "agent_connection.hpp"
#include "dispatcher/i_server_dispatcher.hpp"
#include "dispatcher/server_dispatcher.hpp"
#include "logger.hpp"
#include "poller/i_poller.hpp"
#include "session_manager.hpp"
#include "tcp_server.hpp"

class Reactor {
 public:
  explicit Reactor(TcpServer& server, IServerDispatcher& dispatcher,
                   IPoller& poller, SessionManager& manager)
      : dispatcher_(dispatcher),
        server_(server),
        poller_(poller),
        sessionManager_(manager) {}
  Reactor() = delete;
  Reactor(const Reactor&) = delete;
  Reactor& operator=(const Reactor&) = delete;
  ~Reactor() = default;

  void run();
  void stop() { running_ = false; };

 private:
  IServerDispatcher& dispatcher_;
  TcpServer& server_;
  IPoller& poller_;
  SessionManager& sessionManager_;
  bool running_ = false;
  //   std::unordered_map<int, AgentConnection> agents_;

  void onNewConnection();
  void onAgentReady(int fileDescriptor);
  void onAgentDisconnected(int fileDescriptor);
};

#endif