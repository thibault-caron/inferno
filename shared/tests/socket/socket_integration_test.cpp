#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "socket/socket_factory.hpp"

#define TEST_PORT 9876

// Helper: runs a server that accepts one agent, receives data, sends it back
// "echoes" everything it receives — classic test pattern
void runEchoServer(uint16_t port, bool& serverReady) {
  auto serverSocket = SocketFactory::createTCP();
  serverSocket->bind(port);
  serverSocket->listen();
  serverReady = true;  // signal the main thread we're ready

  auto agentSocket = serverSocket->accept();  // blocks until agent connects

  std::vector<uint8_t> buffer(1024);
  auto receiveResult = agentSocket->recv(buffer.data(), buffer.size());

  // Echo back exactly what we received
  agentSocket->send(buffer.data(), receiveResult.bytesTransferred);
}

TEST(SocketIntegration, SendAndReceiveData) {
  // const uint16_t TEST_PORT = 9876;
  bool serverReady = false;
  // TODO how not to use thread while still be os agnostic (::socketpair() on
  // linux) ? Run the server in a background thread (otherwise it would block
  // our test)
  std::thread serverThread(runEchoServer, static_cast<uint16_t>(TEST_PORT),
                           std::ref(serverReady));

  // Wait until server is actually listening before we try to connect
  while (!serverReady) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Agent side
  auto agentSocket = SocketFactory::createTCP();
  bool connected = agentSocket->connect("127.0.0.1", TEST_PORT);
  ASSERT_TRUE(connected) << "Agent failed to connect";

  // Send some bytes
  std::vector<uint8_t> dataToSend = {0x01, 0x02, 0x03, 0x04};
  auto sendResult = agentSocket->send(dataToSend);
  EXPECT_TRUE(sendResult.ok());
  EXPECT_EQ(sendResult.bytesTransferred, 4);

  // Receive the echo
  std::vector<uint8_t> received(1024);
  auto receiveResult = agentSocket->recv(received.data(), received.size());
  EXPECT_TRUE(receiveResult.ok());
  EXPECT_EQ(receiveResult.bytesTransferred, 4);

  // Check the bytes match exactly
  EXPECT_EQ(received[0], 0x01);
  EXPECT_EQ(received[1], 0x02);
  EXPECT_EQ(received[2], 0x03);
  EXPECT_EQ(received[3], 0x04);

  serverThread.join();
}

TEST(SocketIntegration, ConnectToNonExistentServer) {
  auto socket = SocketFactory::createTCP();
  bool connected =
      socket->connect("127.0.0.1", 19999);  // nothing listening here
  EXPECT_FALSE(connected);
}