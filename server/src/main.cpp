#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include "agent_session.hpp"
#include "server_dispatcher.hpp"
#include "env_helper.hpp"
#include "socket/ISocket.hpp"
#include "socket/socket_factory.hpp"
#include "socket/socket_helper.hpp"
#include "tcp_server.hpp"


int main() {
  // Disable buffering on stdout and stderr to ensure logs are flushed
  // immediately (so we can see them in real-time)
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  const uint16_t serverPort = EnvHelper::resolveServerPort();
  TcpServer server(serverPort);
  // 1. Start — TcpServer owns the listening socket
  if (!server.start()) {
    std::cerr << "[server] Failed to bind/listen on port "
              << serverPort << '\n';
    return 1;
  }
  server.setNonBlocking();
  std::cout << "[server] Listening on port " << serverPort << '\n';

  std::unordered_map<int, AgentSession> agents;
  std::vector<int> disconnected;
  ServerDispatcher dispatcher;
  bool running = true;

  while (running) {
    // Accept new agents (non-blocking — returns nullptr if no pending
    // connection, so existing agents aren't starved)
    std::unique_ptr<ISocket> incoming = server.acceptAgent();
    if (incoming) {
      const int fd = incoming->getFd();
      std::cout << "[server] Agent connected: " << incoming->remoteAddress()
                << ':' << incoming->remotePort() << '\n';
      agents.emplace(fd, AgentSession(std::move(incoming)));
    }

    // ── Serve existing agents ─────────────────
    for (auto& [agentFd, session] : agents) {
      const SocketResult result = SocketHelper::receiveIntoBuffer(session);
      if (!result.ok() || result.error == SocketStatus::CONNECTION_RESET ||
          result.bytesTransferred <= 0) {
        std::cout << "[server] Agent " << agentFd << " disconnected\n";
        disconnected.push_back(agentFd);
        continue;
      }
      while (std::optional<Frame> frame = session.tryExtractFrame()) {
        if (!session.isRegistered() &&
            frame->header.type != MessageType::REGISTER) {
          dispatcher.sendError(session, ErrorType::INVALID_FORMAT,
                               "First message must be REGISTER");
          continue;
        }
        dispatcher.handleFrame(session, frame.value());
      }
    }

    for (const int agentFd : disconnected) {
      agents.erase(agentFd);
    }
    disconnected.clear();

    // to reduce CPU usage when no agents are connected — can be optimized with
    // select()/poll()/epoll() instead of sleeping
    if (agents.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  return 0;
}