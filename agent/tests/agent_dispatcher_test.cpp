#include "agent_dispatcher.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/mock_socket_helpers.hpp"

TEST(AgentDispatcherTest, should_send_response_on_os_info_command) {
  auto [agentSession, mockSocket] = makeAgentSessionWithMock();
  std::vector<std::uint8_t> sentBytes;
  EXPECT_CALL(mockSocket.get(), send(::testing::_, ::testing::_))
      .WillOnce(CaptureSentBytes(sentBytes));

  CommandPayload command;
  command.id = 7;
  command.type = CommandType::OS_INFO;
  command.data = "";
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);

  AgentDispatcher dispatcher;
  dispatcher.handleFrame(agentSession,
                         makeFrame(MessageType::COMMAND, payload));

  ASSERT_GE(sentBytes.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  const std::vector<std::uint8_t> headerBytes(
      sentBytes.begin(), sentBytes.begin() + LPTF_HEADER_SIZE);
  const LptfHeader header = ProtocolParser::parseHeader(headerBytes);
  EXPECT_EQ(header.type, MessageType::RESPONSE);

  const std::vector<std::uint8_t> responseBytes(
      sentBytes.begin() + LPTF_HEADER_SIZE, sentBytes.end());
  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(responseBytes);
  EXPECT_EQ(response.id, 7);
  EXPECT_EQ(response.status, ResponseStatus::OK);
  EXPECT_EQ(response.total_chunks, 1);
  EXPECT_EQ(response.chunk_index, 0);
  EXPECT_EQ(response.data, "hello world from agent");
}

TEST(AgentDispatcherTest, should_close_socket_on_disconnect) {
  auto [agentSession, mockSocket] = makeAgentSessionWithMock();
  EXPECT_CALL(mockSocket.get(), send(::testing::_, ::testing::_)).Times(0);
  EXPECT_CALL(mockSocket.get(), close()).Times(1);

  AgentDispatcher dispatcher;
  dispatcher.handleFrame(agentSession, makeFrame(MessageType::DISCONNECT));

  EXPECT_FALSE(agentSession.isValid());
}
