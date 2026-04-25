#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "client_session.hpp"
#include "dispatcher.hpp"
#include "socket/ISocket.hpp"
#include "tcp_server.hpp"
#include "exception/lptf_exception.hpp"

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

  clients.emplace(fd, ClientSession(std::move(client)));
  std::vector<int> deconnectedClients;
  Dispatcher dispatcher;
  while (true) {
    // for (auto it = clients.begin(); it != clients.end();)
    for (auto& [clientFd, session] : clients) {
      const SocketResult result =
          prealocate_ressources_for_buffer(session, 4096);

      if (result.error == SocketStatus::CONNECTION_RESET || !result.ok()) {
        std::cout << "Client " << clientFd << " disconnected\n";
        deconnectedClients.push_back(clientFd);
        continue;
      }

      while (std::optional<Frame> frame = session.tryExtractFrame()) {
        // Dispatcher dispatcher(session.socket);

        if (!session.isRegistered() &&
            frame->header.type != MessageType::REGISTER) {
          dispatcher.sendError(session, ErrorType::INVALID_FORMAT,
                               "First message must be REGISTER");
          continue;
        }

        // if (!session.isRegistered()) {
        //   if (frame->header.type != MessageType::REGISTER) {
        //     dispatcher.sendError(ErrorType::INVALID_FORMAT,
        //                          "First message must be REGISTER");
        //     continue;
        //   }
        // }

        // is this check necesary ? Does the loop do it for us ?
        if (!frame.has_value()) {
          // here what kind of error we throw and what we do ?
          throw InfernoException("frame has no value", 0);
        }

        dispatcher.dispatch(session, frame.value());
      }

      // ++it;
    }

    for (int clientFd : deconnectedClients) {
      clients.erase(clientFd);
    }
  }

  return 0;
}