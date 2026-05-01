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
#include "dispatcher.hpp"
#include "socket/ISocket.hpp"
#include "socket/SocketFactory.hpp"
#include "tcp_server.hpp"

namespace {
constexpr std::size_t kReceiveChunkSize = 4096;
// std::uint16_t resolveServerPort() {
//   const char* rawPort = std::getenv("SERVER_PORT");
//   if (!rawPort || rawPort[0] == '\0') {
//     std::cout << "SERVER_PORT not found in .env, using default " <<
//     SERVER_PORT
//               << '\n';
//     return SERVER_PORT;
//   }

//   char* end = nullptr;
//   errno = 0;
//   const long parsed = std::strtol(rawPort, &end, 10);
//   if (errno == 0 && end != rawPort && *end == '\0' && parsed > 0 &&
//       parsed <= 65535) {
//     return static_cast<std::uint16_t>(parsed);
//   }

//   std::cout << "Invalid SERVER_PORT value in .env: " << rawPort
//             << ", using default " << SERVER_PORT << '\n';
//   return SERVER_PORT;
// }

}  // namespace

SocketResult receiveIntoBuffer(AgentSession& session) {
  std::vector<std::uint8_t> temp(kReceiveChunkSize);
  const SocketResult result = session.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    session.buffer.insert(session.buffer.end(), temp.begin(), temp.end());
    std::cout << "[agent] recv bytes=" << result.bytesTransferred
              << " buffer_size=" << session.buffer.size() << "\n";
  } else {
    std::cout << "[agent] recv status=" << static_cast<int>(result.error)
              << " bytes=" << result.bytesTransferred << "\n";
  }

  return result;
}

int main() {
  // Disable buffering on stdout and stderr to ensure logs are flushed
  // immediately (so we can see them in real-time)
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);

  TcpServer server(SERVER_PORT);
  // 1. Start — TcpServer owns the listening socket
  if (!server.start()) {
    std::cerr << "[server] Failed to bind/listen on port " << SERVER_PORT
              << '\n';
    return 1;
  }
  server.setNonBlocking();
  std::cout << "[server] Listening on port " << SERVER_PORT << '\n';

  std::unordered_map<int, AgentSession> agents;
  std::vector<int> disconnected;
  Dispatcher dispatcher;
  bool running = true;

  while (running) {
    // Accept new agents (non-blocking — returns nullptr if no pending connection, so existing agents aren't starved)
    std::unique_ptr<ISocket> incoming = server.acceptAgent();
    if (incoming) {
      const int fd = incoming->getFd();
      std::cout << "[server] Agent connected: " << incoming->remoteAddress()
                << ':' << incoming->remotePort() << '\n';
      agents.emplace(fd, AgentSession(std::move(incoming)));
    }


    // ── Serve existing agents ─────────────────
    for (auto& [agentFd, session] : agents) {
      const SocketResult result = receiveIntoBuffer(session);
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
        dispatcher.dispatch(session, frame.value());
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

///////////////////////

// #include <iostream>
// #include <memory>
// #include <optional>
// #include <unordered_map>
// #include <vector>

// #include "agent_session.hpp"
// #include "dispatcher.hpp"
// #include "socket/ISocket.hpp"
// #include "tcp_server.hpp"

// // ── TODO: move to shared/ ─────────────────────────────────────
// // Same function exists in agent/main.cpp — duplication to be
// // resolved once the simple flow works end-to-end.
// static SocketResult receiveIntoBuffer(AgentSession& session) {
//   constexpr std::size_t kChunkSize = 4096;
//   std::vector<std::uint8_t> temp(kChunkSize);
//   const SocketResult result = session.socket->recv(temp.data(), temp.size());

//   if (result.ok() && result.bytesTransferred > 0) {
//     temp.resize(static_cast<std::size_t>(result.bytesTransferred));
//     session.buffer.insert(session.buffer.end(), temp.begin(), temp.end());
//   }
//   return result;
// }
// // ─────────────────────────────────────────────────────────────

// int main() {
//   // ① Start — TcpServer owns the listening socket
//   TcpServer server(SERVER_PORT);
//   std::cout << "[server] Starting on port " << SERVER_PORT << '\n';
//   if (!server.start()) {
//     std::cerr << "[server] Failed to bind/listen on port "
//               << SERVER_PORT << '\n';
//     return 1;
//   }
//   std::cout << "[server] Listening on port " << SERVER_PORT << '\n';

//   // ② Accept — blocks until one agent connects
//   std::unique_ptr<ISocket> agentSocket = server.acceptAgent();
//   if (!agentSocket) {
//     std::cerr << "[server] accept() failed\n";
//     return 1;
//   }
//   std::cout << "[server] Agent connected: "
//             << agentSocket->remoteAddress() << ':'
//             << agentSocket->remotePort() << '\n';

//   // ③ Session + dispatcher setup
//   const int fd = agentSocket->getFd();
//   std::unordered_map<int, AgentSession> agents;
//   agents.emplace(fd, AgentSession(std::move(agentSocket)));

//   Dispatcher dispatcher;
//   std::vector<int> disconnected;

//   // ④ Main loop
//   while (!agents.empty()) {
//     for (auto& [agentFd, session] : agents) {
//       const SocketResult result = receiveIntoBuffer(session);

//       if (!result.ok() ||
//           result.error == SocketStatus::CONNECTION_RESET ||
//           result.bytesTransferred <= 0) {
//         std::cout << "[server] Agent " << agentFd << " disconnected\n";
//         disconnected.push_back(agentFd);
//         continue;
//       }

//       while (std::optional<Frame> frame = session.tryExtractFrame()) {
//         // Enforce REGISTER-first rule before dispatching
//         if (!session.isRegistered() &&
//             frame->header.type != MessageType::REGISTER) {
//           dispatcher.sendError(session, ErrorType::INVALID_FORMAT,
//                                "First message must be REGISTER");
//           continue;
//         }
//         dispatcher.dispatch(session, frame.value());
//       }
//     }

//     for (const int agentFd : disconnected) {
//       agents.erase(agentFd);
//     }
//     disconnected.clear();
//   }

//   std::cout << "[server] No agents connected, exiting\n";
//   return 0;
// }
