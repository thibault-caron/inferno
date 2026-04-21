#include <iostream>

#include "socket/ISocket.hpp"
#include "tcp_server.hpp"

int main() {
  TcpServer server(SERVER_PORT);
  if (!server.start()) {
    std::cerr << "Failed to create listening socket on port " << SERVER_PORT
              << '\n';
    return 1;
  }

  std::cout << "Tcp Server ready on port " << SERVER_PORT << '\n';
  return 0;
}