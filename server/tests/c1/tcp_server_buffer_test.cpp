#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <vector>

#include "c1_test_helpers.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/SocketFactory.hpp"
#include "tcp_server.hpp"

namespace {
constexpr std::uint16_t kPortBuffer = 12082;
}

TEST(TcpServerBuffer, should_keep_partial_frame_until_complete) {
  TcpServer server(kPortBuffer);
  ASSERT_TRUE(server.start());

  std::unique_ptr<ISocket> client = SocketFactory::createTCP();
  ASSERT_TRUE(client->connect("127.0.0.1", kPortBuffer));

  std::unique_ptr<ISocket> acceptedClient = server.acceptClient();
  ASSERT_NE(acceptedClient, nullptr);

  const std::vector<std::uint8_t> frame = makeRegisterFrame("agent-a");
  const std::size_t splitPos = 5;

  std::vector<std::uint8_t> firstChunk(frame.begin(), frame.begin() + splitPos);
  std::vector<std::uint8_t> secondChunk(frame.begin() + splitPos, frame.end());

  EXPECT_TRUE(client->send(firstChunk).ok());
  EXPECT_TRUE(server.receiveFromClient(*acceptedClient).ok());
  EXPECT_EQ(server.parseAvailableFrames(), 0);
  EXPECT_EQ(server.parsedFrames().size(), 0);
  EXPECT_EQ(server.receiveBuffer().size(), firstChunk.size());

  EXPECT_TRUE(client->send(secondChunk).ok());
  EXPECT_TRUE(server.receiveFromClient(*acceptedClient).ok());
  EXPECT_EQ(server.parseAvailableFrames(), 1);
  ASSERT_EQ(server.parsedFrames().size(), 1);

  const TcpServer::ParsedFrame& parsed = server.parsedFrames().front();
  EXPECT_EQ(parsed.header.type, MessageType::REGISTER);

  const RegisterPayload payload =
      ProtocolParser::parseRegisterPayload(parsed.payload);
  EXPECT_EQ(payload.hostname, "agent-a");
  EXPECT_EQ(server.receiveBuffer().size(), 0);
}

TEST(TcpServerBuffer, should_parse_two_frames_in_single_recv) {
  TcpServer server(kPortBuffer + 1);
  ASSERT_TRUE(server.start());

  std::unique_ptr<ISocket> client = SocketFactory::createTCP();
  ASSERT_TRUE(client->connect("127.0.0.1", kPortBuffer + 1));

  std::unique_ptr<ISocket> acceptedClient = server.acceptClient();
  ASSERT_NE(acceptedClient, nullptr);

  const std::vector<std::uint8_t> frameA = makeRegisterFrame("alpha");
  const std::vector<std::uint8_t> frameB = makeRegisterFrame("beta");

  std::vector<std::uint8_t> merged;
  merged.reserve(frameA.size() + frameB.size());
  merged.insert(merged.end(), frameA.begin(), frameA.end());
  merged.insert(merged.end(), frameB.begin(), frameB.end());

  EXPECT_TRUE(client->send(merged).ok());
  EXPECT_TRUE(server.receiveFromClient(*acceptedClient).ok());
  EXPECT_EQ(server.parseAvailableFrames(), 2);
  ASSERT_EQ(server.parsedFrames().size(), 2);

  const RegisterPayload first =
      ProtocolParser::parseRegisterPayload(server.parsedFrames()[0].payload);
  const RegisterPayload second =
      ProtocolParser::parseRegisterPayload(server.parsedFrames()[1].payload);

  EXPECT_EQ(first.hostname, "alpha");
  EXPECT_EQ(second.hostname, "beta");
  EXPECT_EQ(server.receiveBuffer().size(), 0);
}
