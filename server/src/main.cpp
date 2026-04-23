#include <iostream>
#include <memory>
#include <unordered_map>

#include "client_session.hpp"
#include "socket/ISocket.hpp"
#include "tcp_server.hpp"

int main() {
  TcpServer server(SERVER_PORT);

  if (!server.start()) {
    std::cerr << "Failed to bind/listen on port " << SERVER_PORT << '\n';
    return 1;
  }
  std::cout << "Listening on port " << SERVER_PORT << '\n';

  std::unordered_map<int, std::unique_ptr<ClientSession>> clients;

  // Only one client for now, but the pattern is already multi-client ready
  std::unique_ptr<ISocket> client = server.acceptClient();
  if (!client) {
    std::cerr << "accept() failed\n";
    return 1;
  }

  const int fd = client->getFd();  // we need to expose this — see below

  std::cout << "Client connected: " << client->remoteAddress() << ':'
            << client->remotePort() << '\n';

  // ClientSession clientSession(std::move(client));

  clients.emplace(fd, std::make_unique<ClientSession>(std::move(client)));

  while (!clients.empty()) {
    // Later: epoll tells you WHICH fds are readable, you iterate only those.
    // For now: one client, so we just iterate all.
    for (auto& [clientFd, client] : clients) {
      SocketResult result = client->socket->recv(client->buffer);

      if (result.error == SocketStatus::CONNECTION_RESET || !result.ok()) {
        std::cout << "Client " << clientFd << " disconnected\n";
        clients.erase(clientFd);
        break;  // iterator invalidated, restart outer loop
      }

      std::optional<Frame> frame = client->tryExtractFrame();
      std::cout << "Frame from client " << clientFd
                << ", payload size: " << frame->payload.size() << '\n';

      // while (std::optional<Frame> frame = client->tryExtractFrame()) {
      //     std::cout << "Frame from client " << clientFd
      //               << ", payload size: " << frame->payload.size() << '\n';
      // }
    }
  }

  return 0;
}