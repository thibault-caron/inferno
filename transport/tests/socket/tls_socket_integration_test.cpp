#include <gtest/gtest.h>

#include <future>
#include <string>
#include <thread>
#include <vector>

#include "fixtures/common.hpp"
#include "fixtures/ports.hpp"
#include "socket/socket_factory.hpp"
#include "socket/tls_socket_factory.hpp"

class TLSSocketIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);  // Unix only: ignore broken pipe
#endif

    // signal(SIGPIPE, SIG_IGN);
  }
};

TEST_F(TLSSocketIntegrationTest,
       should_echo_message_when_both_endpoints_use_tls) {
  std::promise<void> serverReady;
  std::future<void> serverReadyFuture = serverReady.get_future();

  std::thread serverThread([&]() {
    auto server =
        TLSSocketFactory::createServer(Tls::SERVER_CERT, Tls::SERVER_KEY);
    server->bind(Ports::Tls::ECHO_PORT);
    server->listen();

    serverReady.set_value();  // unblocks the client

    auto connection = server->accept();

    std::vector<std::uint8_t> buffer(256);
    auto result = connection->recv(buffer.data(), buffer.size());

    // echo back exactly what was received
    connection->send(buffer.data(), result.bytesTransferred);
    connection->close();
  });

  serverReadyFuture.wait();  // client waits here until server is listening

  auto client = TLSSocketFactory::createClient(Tls::CA_CERT);
  ASSERT_TRUE(client->connect(Common::SERVER_HOST, Ports::Tls::ECHO_PORT));

  const std::string message = "hello world";
  auto sendResult = client->send(
      reinterpret_cast<const std::uint8_t*>(message.data()), message.size());
  EXPECT_TRUE(sendResult.ok());

  std::vector<std::uint8_t> buffer(256);
  auto recvResult = client->recv(buffer.data(), buffer.size());
  EXPECT_TRUE(recvResult.ok());

  std::string received(buffer.begin(),
                       buffer.begin() + recvResult.bytesTransferred);
  EXPECT_EQ(received, message);

  client->close();
  serverThread.join();
}

TEST_F(TLSSocketIntegrationTest,
       should_fail_to_connect_when_server_does_not_use_tls) {
  std::promise<void> serverReady;
  std::future<void> serverReadyFuture = serverReady.get_future();

  std::thread serverThread([&]() {
    // plain TCP server — no TLS
    auto server = SocketFactory::createTCP();
    server->bind(Ports::Tls::PLAIN_SERVER_PORT);
    server->listen();
    serverReady.set_value();
    server->accept();  // accepts TCP but speaks no TLS
  });

  serverReadyFuture.wait();

  // TLS client tries to connect — SSL_connect will fail
  // because server sends no TLS handshake back
  auto client = TLSSocketFactory::createClient(Tls::CA_CERT);
  EXPECT_FALSE(
      client->connect(Common::SERVER_HOST, Ports::Tls::PLAIN_SERVER_PORT));
  client->close();
  serverThread.join();
}

TEST_F(TLSSocketIntegrationTest,
       should_fail_to_accept_when_client_does_not_use_tls) {
  std::promise<void> serverReady;
  std::future<void> serverReadyFuture = serverReady.get_future();

  std::unique_ptr<ISocket> accepted;

  std::thread serverThread([&]() {
    auto server =
        TLSSocketFactory::createServer(Tls::SERVER_CERT, Tls::SERVER_KEY);
    server->bind(Ports::Tls::PLAIN_CLIENT_PORT);
    server->listen();
    serverReady.set_value();

    // SSL_accept will fail — plain client sends no ClientHello
    accepted = server->accept();
  });

  serverReadyFuture.wait();

  // plain TCP client — connects at TCP level but speaks no TLS
  auto client = SocketFactory::createTCP();
  client->connect(Common::SERVER_HOST, Ports::Tls::PLAIN_CLIENT_PORT);

  client->close();
  serverThread.join();

  EXPECT_EQ(accepted, nullptr);
}