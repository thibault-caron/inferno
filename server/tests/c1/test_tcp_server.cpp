#include <arpa/inet.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "socket/ISocket.hpp"
#include "tcp_server.hpp"

// ─────────────────────────────────────────────────────────────
// Unit tests (no real network)
// ─────────────────────────────────────────────────────────────

TEST(TcpServerUnit,
     should_return_false_when_start_called_on_already_used_port) {
  // Occupy the port before TcpServer tries to bind.
  int occupier = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(occupier, -1);

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(19876);
  ASSERT_EQ(
      ::bind(occupier, reinterpret_cast<sockaddr*>(&address), sizeof(address)),
      0);
  ASSERT_EQ(::listen(occupier, 1), 0);

  TcpServer server(19876);
  EXPECT_FALSE(server.start());

  ::close(occupier);
}

TEST(TcpServerUnit,
     should_return_nullptr_when_acceptClient_called_before_start) {
  TcpServer server(19877);
  // start() was never called — serverSocket_ is null
  EXPECT_EQ(server.acceptClient(), nullptr);
}

// ─────────────────────────────────────────────────────────────
// Integration tests (real loopback TCP)
// ─────────────────────────────────────────────────────────────

// Helper: open a raw loopback socket and connect to host:port.
static int connectLoopback(std::uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) return -1;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  ::inet_pton(AF_INET, "localhost", &address.sin_addr);
  if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
      0) {
    ::close(fd);
    return -1;
  }
  return fd;
}

TEST(TcpServerIntegration,
     should_return_true_when_start_is_called_on_available_port) {
  TcpServer server(19872);
  EXPECT_TRUE(server.start());
}

TEST(TcpServerIntegration,
     should_return_false_when_start_is_called_a_second_time_on_same_instance) {
  TcpServer server(19873);
  ASSERT_TRUE(server.start());
  // Le deuxième start() va tenter de re-créer et binder le socket.
  // Comme le port est occupé par la première instance, ça doit échouer.
  EXPECT_FALSE(server.start());
}

TEST(TcpServerIntegration,
     should_return_valid_socket_when_client_connects_after_start) {
  constexpr std::uint16_t port = 19879;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  // Connect from a background thread so accept() doesn't block the test.
  int clientFd = -1;
  std::thread connector([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    clientFd = connectLoopback(port);
  });

  auto accepted = server.acceptClient();
  connector.join();

  EXPECT_NE(accepted, nullptr);
  EXPECT_TRUE(accepted->isValid());

  if (clientFd != -1) ::close(clientFd);
}

TEST(TcpServerIntegration, should_echo_data_back_through_accepted_socket) {
  constexpr std::uint16_t port = 19880;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  // ── Client thread: send 4 bytes, expect them echoed back ──
  std::vector<std::uint8_t> received;
  std::thread client([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int fd = connectLoopback(port);
    ASSERT_NE(fd, -1);
    const std::uint8_t message[] = {0x01, 0x02, 0x03, 0x04};
    ::send(fd, message, sizeof(message), 0);
    std::uint8_t buffer[4];
    ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
    if (n > 0) received.assign(buffer, buffer + n);
    ::close(fd);
  });

  // ── Server side: accept, receive, echo back ───────────────
  auto socket = server.acceptClient();
  ASSERT_NE(socket, nullptr);

  std::vector<std::uint8_t> temp(4);
  const SocketResult result = socket->recv(temp.data(), temp.size());
  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.bytesTransferred, 4);
  socket->send(temp.data(), result.bytesTransferred);  // echo

  client.join();

  EXPECT_EQ(received, (std::vector<std::uint8_t>{0x01, 0x02, 0x03, 0x04}));
}

TEST(TcpServerIntegration,
     should_return_valid_remote_address_for_accepted_client) {
  constexpr std::uint16_t port = 19881;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  std::thread connector([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int fd = connectLoopback(port);
    if (fd != -1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      ::close(fd);
    }
  });

  auto socket = server.acceptClient();
  connector.join();

  ASSERT_NE(socket, nullptr);
  EXPECT_EQ(socket->remoteAddress(), "127.0.0.1");  // "localhost" does not work
  EXPECT_GT(socket->remotePort(), 0);
}
