#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "client_session.hpp"
#include "dispatcher.hpp"
#include "exception/lptf_exception.hpp"
#include "socket/ISocket.hpp"
#include "tcp_server.hpp"

SocketResult prealocate_ressources_for_buffer(ClientSession& client,
                                              std::size_t ressource_size) {
  std::vector<std::uint8_t> temp(ressource_size);
  SocketResult result = client.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    client.buffer.insert(client.buffer.end(), temp.begin(), temp.end());
  }

  return result;
}

int main() {
  TcpServer server(SERVER_PORT);

  if (!server.start()) {
    std::cerr << "Failed to bind/listen on port " << SERVER_PORT << '\n';
    return 1;
  }
  std::cout << "Listening on port " << SERVER_PORT << '\n';

  std::unordered_map<int, ClientSession> clients;

  // Accept the first client for now; the loop below is ready for more clients
  // once accept() becomes part of a non-blocking poll/select/epoll flow.
  std::unique_ptr<ISocket> client = server.acceptClient();
  if (!client) {
    std::cerr << "accept() failed\n";
    return 1;
  }

  const int fd = client->getFd();

  std::cout << "Client connected: " << client->remoteAddress() << ':'
            << client->remotePort() << '\n';

  // clients.emplace(fd, std::make_unique<ClientSession>(std::move(client)));
  clients.emplace(fd, ClientSession(std::move(client)));
  std::vector<int> disconnectedClients;
  Dispatcher dispatcher;

  bool running = true;
  // set running = false on SIGINT via signal handler

  while (running) {
    for (auto& [clientFd, clientSession] : clients) {
      // if (!clientSession) {
      //   disconnectedClients.push_back(clientFd);
      //   continue;
      // }

      // ClientSession& session = *sessionPtr;
      const SocketResult result =
          prealocate_ressources_for_buffer(clientSession, 4096);

      if (result.error == SocketStatus::CONNECTION_RESET || !result.ok()) {
        std::cout << "Client " << clientFd << " disconnected\n";
        disconnectedClients.push_back(clientFd);
        continue;
      }

      while (std::optional<Frame> frame = clientSession.tryExtractFrame()) {
        // Dispatcher dispatcher(session.socket);

        if (!clientSession.isRegistered() &&
            frame->header.type != MessageType::REGISTER) {
          dispatcher.sendError(clientSession, ErrorType::INVALID_FORMAT,
                               "First message must be REGISTER");
          continue;
        }

        dispatcher.dispatch(clientSession, frame.value());
      }
    }

    for (int clientFd : disconnectedClients) {
      clients.erase(clientFd);
    }
    disconnectedClients.clear();
  }

  return 0;
}