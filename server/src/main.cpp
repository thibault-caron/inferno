#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "agent_session.hpp"
#include "dispatcher.hpp"
#include "socket/ISocket.hpp"
#include "tcp_server.hpp"

namespace {

std::uint16_t resolveServerPort() {
  const char* rawPort = std::getenv("SERVER_PORT");
  if (!rawPort || rawPort[0] == '\0') {
    return SERVER_PORT;
  }

  char* end = nullptr;
  errno = 0;
  const long parsed = std::strtol(rawPort, &end, 10);
  if (errno == 0 && end != rawPort && *end == '\0' && parsed > 0 &&
      parsed <= 65535) {
    return static_cast<std::uint16_t>(parsed);
  }

  return SERVER_PORT;
}

}  // namespace

SocketResult prealocate_ressources_for_buffer(AgentSession& agent,
                                              std::size_t ressource_size) {
  std::vector<std::uint8_t> temp(ressource_size);
  const SocketResult result = agent.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    agent.buffer.insert(agent.buffer.end(), temp.begin(), temp.end());
  }

  return result;
}

int main() {
  // TcpServer server(SERVER_PORT);
  const std::uint16_t serverPort = resolveServerPort();
  TcpServer server(serverPort);

  if (!server.start()) {
    std::cerr << "Failed to bind/listen on port " << SERVER_PORT << '\n';
    return 1;
  }
  std::cout << "Listening on port " << SERVER_PORT << '\n';

  std::unordered_map<int, AgentSession> agents;

  // Accept the first agent for now; the loop below is ready for more agents
  // once accept() becomes part of a non-blocking poll/select/epoll flow.
  std::unique_ptr<ISocket> agent = server.acceptAgent();
  if (!agent) {
    std::cerr << "accept() failed\n";
    return 1;
  }

  const int fd = agent->getFd();

  std::cout << "Agent connected: " << agent->remoteAddress() << ':'
            << agent->remotePort() << '\n';

  // agents.emplace(fd, std::make_unique<AgentSession>(std::move(agent)));
  agents.emplace(fd, AgentSession(std::move(agent)));
  std::vector<int> disconnectedAgents;
  Dispatcher dispatcher;

  bool running = true;
  // set running = false on SIGINT via signal handler

  while (running) {
    for (auto& [agentFd, agentSession] : agents) {
      // if (!agentSession) {
      //   disconnectedAgents.push_back(agentFd);
      //   continue;
      // }

      // AgentSession& session = *sessionPtr;
      const SocketResult result =
          prealocate_ressources_for_buffer(session, 4096);

      if (result.error == SocketStatus::CONNECTION_RESET || !result.ok()) {
        std::cout << "Agent " << agentFd << " disconnected\n";
        disconnectedAgents.push_back(agentFd);
        continue;
      }

      while (std::optional<Frame> frame = agentSession.tryExtractFrame()) {
        // Dispatcher dispatcher(session.socket);

        if (!agentSession.isRegistered() &&
            frame->header.type != MessageType::REGISTER) {
          dispatcher.sendError(session, ErrorType::INVALID_FORMAT,
                               "First message must be REGISTER");
          continue;
        }

        dispatcher.dispatch(session, frame.value());
      }

    }

    for (int agentFd : disconnectedAgents) {
      agents.erase(agentFd);
    }
    disconnectedAgents.clear();
  }

  return 0;
}