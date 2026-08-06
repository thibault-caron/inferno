#include <gtest/gtest.h>

#include <future>
#include <thread>
#include <vector>

#include "fixtures/common.hpp"
#include "fixtures/ports.hpp"
#include "network/socket_test_utils.hpp"
#include "socket/socket_factory.hpp"

TEST(SocketIntegration, should_echo_sent_bytes_back_to_agent) {
  std::promise<void> serverReady;
  std::thread serverThread(Network::runEchoServer, Ports::Socket::ECHO_PORT,
                           std::ref(serverReady));
  serverReady.get_future().wait();  // precise — no sleep, no busy-wait

  auto agentSocket = SocketFactory::createTCP();
  bool connected =
      agentSocket->connect(Common::SERVER_HOST, Ports::Socket::ECHO_PORT);
  ASSERT_TRUE(connected) << " Agent failed to connect to echo server on port "
                         << Ports::Socket::ECHO_PORT;

  const std::vector<std::uint8_t> dataToSend = {0x01, 0x02, 0x03, 0x04};
  auto sendResult = agentSocket->send(dataToSend);
  EXPECT_TRUE(sendResult.ok());
  EXPECT_EQ(sendResult.bytesTransferred, 4);

  std::vector<std::uint8_t> received(1024);
  auto recvResult = agentSocket->recv(received.data(), received.size());
  EXPECT_TRUE(recvResult.ok());
  EXPECT_EQ(recvResult.bytesTransferred, 4);

  received.resize(static_cast<std::size_t>(recvResult.bytesTransferred));
  EXPECT_EQ(received, dataToSend);

  serverThread.join();
}

TEST(SocketIntegration,
     should_fail_to_connect_when_nothing_is_listening_on_port) {
  auto socket = SocketFactory::createTCP();
  bool connected =
      socket->connect(Common::SERVER_HOST, Ports::Socket::UNUSED_PORT);
  EXPECT_FALSE(connected);
}