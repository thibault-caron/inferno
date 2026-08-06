#ifndef SOCKET_HELPER_HPP
#define SOCKET_HELPER_HPP

#include <cstdint>
#include <future>
#include <thread>

#include "fixtures/common.hpp"
#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "stubs/test_tcp_server.hpp"

namespace Network {

struct ConnectedSockets {
  std::unique_ptr<ISocket> serverSide;
  std::unique_ptr<ISocket> clientSide;
};

ConnectedSockets makeConnectedSockets(std::uint16_t port) {
  TestTcpServer server(port);
  if (!server.start()) {
    return {};
  }

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture = serverReady.get_future().share();
  std::unique_ptr<ISocket> client;

  std::thread connector([&] {
    serverReadyFuture.wait();
    client = SocketFactory::createTCP();
    if (client) {
      client->connect(Common::SERVER_HOST, port);
    }
  });

  serverReady.set_value();
  std::unique_ptr<ISocket> serverSide = server.acceptAgent();
  connector.join();

  return {std::move(serverSide), std::move(client)};
}

void closeSockets(ConnectedSockets& sockets) {
  if (sockets.serverSide) {
    sockets.serverSide->close();
  }
  if (sockets.clientSide) {
    sockets.clientSide->close();
  }
}

// Echo server: binds, listens, signals ready, accepts one client,
// receives data, sends it back.
// The promise is set after listen() — at that point the OS is ready
// to queue incoming connections, so the client can safely connect.
void runEchoServer(std::uint16_t port,
                            std::promise<void>& serverReady) {
  auto serverSocket = SocketFactory::createTCP();
  serverSocket->bind(port);
  serverSocket->listen();
  serverReady.set_value();  // unblocks the client — no busy-wait needed

  auto agentSocket = serverSocket->accept();
  std::vector<std::uint8_t> buffer(1024);
  auto receivedResult = agentSocket->recv(buffer.data(), buffer.size());
  agentSocket->send(buffer.data(),
                    static_cast<std::size_t>(receivedResult.bytesTransferred));
}

}  // namespace Network

#endif