#include "tcp_server.hpp"

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "test_constants.hpp"

// All integration tests connect using SocketFactory::createTCP() — the same
// path the real agent uses — rather than raw syscall helpers.

// ── Unit tests (no network) ───────────────────────────────────

TEST(TcpServerUnit,
     should_return_false_when_start_called_on_already_used_port) {
  // Occupy the port first using a raw socket (intentional: we're testing
  // that TcpServer correctly detects a port conflict, not the agent path)
  sockaddr_in address{};
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = htons(TestConstants::TCP_SERVER_OCCUPIED_PORT);
  int occupier = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(occupier, -1);
  ASSERT_EQ(::bind(occupier,
                   reinterpret_cast<sockaddr*>(&address),
                   sizeof(address)), 0);

  TcpServer server(TestConstants::TCP_SERVER_OCCUPIED_PORT);
  EXPECT_FALSE(server.start());

  ::close(occupier);
}

TEST(TcpServerUnit,
     should_return_nullptr_when_acceptAgent_called_before_start) {
  TcpServer server(TestConstants::TCP_SERVER_NOT_STARTED_PORT);
  EXPECT_EQ(server.acceptAgent(), nullptr);
}

// ── Integration tests (real loopback) ────────────────────────

TEST(TcpServerIntegration,
     should_return_true_when_start_is_called_on_available_port) {
  TcpServer server(TestConstants::TCP_SERVER_AVAILABLE_PORT);
  EXPECT_TRUE(server.start());
}

TEST(TcpServerIntegration,
     should_return_false_when_start_is_called_a_second_time) {
  TcpServer server(TestConstants::TCP_SERVER_DOUBLE_START_PORT);
  ASSERT_TRUE(server.start());
  EXPECT_FALSE(server.start());
}

TEST(TcpServerIntegration,
     should_return_valid_socket_when_agent_connects) {
  const std::uint16_t port = TestConstants::TCP_SERVER_AGENT_CONNECT_PORT;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  std::unique_ptr<ISocket> agentSocket;
  std::thread connector([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    agentSocket = SocketFactory::createTCP();
    agentSocket->connect("127.0.0.1", port);
  });

  auto accepted = server.acceptAgent();
  connector.join();

  EXPECT_NE(accepted, nullptr);
  EXPECT_TRUE(accepted->isValid());
}

TEST(TcpServerIntegration,
     should_receive_bytes_sent_by_connected_agent) {
  const std::uint16_t port = TestConstants::TCP_SERVER_ECHO_PORT;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  const std::vector<std::uint8_t> message = {0x01, 0x02, 0x03, 0x04};
  std::vector<std::uint8_t> received;

  std::thread agentThread([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto socket = SocketFactory::createTCP();
    socket->connect("127.0.0.1", port);
    socket->send(message);

    // Read echo back
    std::vector<std::uint8_t> buf(message.size());
    const SocketResult res = socket->recv(buf.data(), buf.size());
    if (res.ok() && res.bytesTransferred > 0) {
      buf.resize(static_cast<std::size_t>(res.bytesTransferred));
      received = buf;
    }
  });

  auto serverSocket = server.acceptAgent();
  ASSERT_NE(serverSocket, nullptr);

  // Echo back what we receive
  std::vector<std::uint8_t> buf(message.size());
  const SocketResult res = serverSocket->recv(buf.data(), buf.size());
  EXPECT_TRUE(res.ok());
  EXPECT_EQ(res.bytesTransferred, static_cast<int>(message.size()));
  serverSocket->send(buf.data(), static_cast<std::size_t>(res.bytesTransferred));

  agentThread.join();
  EXPECT_EQ(received, message);
}

TEST(TcpServerIntegration,
     should_report_loopback_address_for_connected_agent) {
  const std::uint16_t port = TestConstants::TCP_SERVER_REMOTE_ADDR_PORT;
  TcpServer server(port);
  ASSERT_TRUE(server.start());

  std::thread connector([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto socket = SocketFactory::createTCP();
    socket->connect("127.0.0.1", port);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  });

  auto accepted = server.acceptAgent();
  connector.join();

  ASSERT_NE(accepted, nullptr);
  EXPECT_EQ(accepted->remoteAddress(), "127.0.0.1");
  EXPECT_GT(accepted->remotePort(), 0);
}
