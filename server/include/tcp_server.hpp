#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

// #include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include "socket/i_socket.hpp"




class TcpServer {
 public:
  explicit TcpServer(std::uint16_t port, int backlog = 10);
  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;
  ~TcpServer() = default;

  bool start();
  std::unique_ptr<ISocket> acceptAgent() const;

  bool setNonBlocking() {
    return serverSocket_ && serverSocket_->setNonBlocking(true);
  }

 private:
  std::uint16_t port_;
  int backlog_;
  std::unique_ptr<ISocket> serverSocket_;
  std::vector<std::uint8_t> receiveBuffer_;
};

#endif