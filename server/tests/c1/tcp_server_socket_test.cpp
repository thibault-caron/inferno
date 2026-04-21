#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "c1_test_helpers.hpp"
#include "socket/SocketFactory.hpp"
#include "tcp_server.hpp"

namespace {
constexpr std::uint16_t kPortSocket = 12081;
}

TEST(TcpServerSocket, should_accept_connection_and_exchange_bytes) {
  TcpServer server(kPortSocket);
  ASSERT_TRUE(server.start());

  std::unique_ptr<ISocket> client = SocketFactory::createTCP();
  ASSERT_TRUE(client->connect("127.0.0.1", kPortSocket));

  std::unique_ptr<ISocket> acceptedClient = server.acceptClient();
  ASSERT_NE(acceptedClient, nullptr);

  const std::vector<std::uint8_t> outbound{0x01, 0x02, 0x03, 0x04};
  const SocketResult sendResult = client->send(outbound);
  EXPECT_TRUE(sendResult.ok());
  EXPECT_EQ(sendResult.bytesTransferred, 4);

  std::vector<std::uint8_t> incoming(16, 0);
  const SocketResult recvResult =
      acceptedClient->recv(incoming.data(), incoming.size());
  EXPECT_TRUE(recvResult.ok());
  EXPECT_EQ(recvResult.bytesTransferred, 4);

  const std::vector<std::uint8_t> echoed(
      incoming.begin(), incoming.begin() + recvResult.bytesTransferred);
  const SocketResult echoSend = server.sendToClient(*acceptedClient, echoed);
  EXPECT_TRUE(echoSend.ok());
  EXPECT_EQ(echoSend.bytesTransferred, 4);

  std::vector<std::uint8_t> echoedBack(16, 0);
  const SocketResult clientRecv =
      client->recv(echoedBack.data(), echoedBack.size());
  EXPECT_TRUE(clientRecv.ok());
  EXPECT_EQ(clientRecv.bytesTransferred, 4);
  EXPECT_EQ(echoedBack[0], 0x01);
  EXPECT_EQ(echoedBack[1], 0x02);
  EXPECT_EQ(echoedBack[2], 0x03);
  EXPECT_EQ(echoedBack[3], 0x04);
}
