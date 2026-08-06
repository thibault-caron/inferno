#ifndef STUB_TEST_TCP_SERVER_HPP
#define STUB_TEST_TCP_SERVER_HPP

#include <cstdint>
#include <memory>

#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "stubs/test_frame_transport.hpp"

// TestTcpServer — a minimal listen/accept wrapper for integration tests.
//
// Depends only on ISocket and SocketFactory, both in shared/.
// Lives in test_support/stubs/ — available to all modules.
//
// It is NOT TcpServer — it has no logger, no reactor, no dispatcher.
// Its only job: bind a port, accept one connection, hand back the ISocket.
// The test drives the protocol on that socket directly.
class TestTcpServer {
 public:
  explicit TestTcpServer(std::uint16_t port) : port_(port) {}

  // bind() + listen() — returns false if either fails
  bool start() {
    socket_ = SocketFactory::createTCP();
    if (!socket_ || !socket_->isValid()) return false;
    if (!socket_->bind(port_))           return false;
    return socket_->listen(1);
  }

  // Blocks until one client connects, returns its socket.
  // Returns nullptr if start() was never called.
  std::unique_ptr<ISocket> acceptAgent() {
    if (!socket_) return nullptr;
    return socket_->accept();
  }

 private:
  std::uint16_t            port_;
  std::unique_ptr<ISocket> socket_;
};

#endif
