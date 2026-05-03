#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
#include "server_dispatcher.hpp"
#include "socket/mock_socket_helpers.hpp"

TEST(ServerDispatcherTest, should_register_and_send_os_info_command) {
  auto [agentSession, mockSocket] = makeAgentSessionWithMock();
  std::vector<std::uint8_t> sentBytes;
  EXPECT_CALL(mockSocket.get(), send(::testing::_, ::testing::_))
      .WillOnce(CaptureSentBytes(sentBytes));

  ServerDispatcher dispatcher;
  dispatcher.handleFrame(
      agentSession,
      makeFrame(MessageType::REGISTER, makeRegisterPayload("worker-42")));

  EXPECT_TRUE(agentSession.isRegistered());
  EXPECT_EQ(agentSession.getAgentInfo().hostname, "worker-42");

  ASSERT_GE(sentBytes.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  const std::vector<std::uint8_t> headerBytes(
      sentBytes.begin(), sentBytes.begin() + LPTF_HEADER_SIZE);
  const LptfHeader header = ProtocolParser::parseHeader(headerBytes);
  EXPECT_EQ(header.type, MessageType::COMMAND);
  EXPECT_EQ(header.size, sentBytes.size() - LPTF_HEADER_SIZE);

  const std::vector<std::uint8_t> payload(sentBytes.begin() + LPTF_HEADER_SIZE,
                                          sentBytes.end());
  const CommandPayload command = ProtocolParser::parseCommandPayload(payload);
  EXPECT_EQ(command.type, CommandType::OS_INFO);
  EXPECT_EQ(command.id, 0);
}

TEST(ServerDispatcherTest,
     should_send_disconnect_when_last_response_chunk_received) {
  auto [agentSession, mockSocket] = makeAgentSessionWithMock();
  std::vector<std::uint8_t> sentBytes;
  EXPECT_CALL(mockSocket.get(), send(::testing::_, ::testing::_))
      .WillOnce(CaptureSentBytes(sentBytes));

  ServerDispatcher dispatcher;
  dispatcher.handleFrame(
      agentSession,
      makeFrame(MessageType::RESPONSE, makeResponsePayload(7, "done", 1, 0)));

  ASSERT_GE(sentBytes.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  const std::vector<std::uint8_t> headerBytes(
      sentBytes.begin(), sentBytes.begin() + LPTF_HEADER_SIZE);
  const LptfHeader header = ProtocolParser::parseHeader(headerBytes);
  EXPECT_EQ(header.type, MessageType::DISCONNECT);
  EXPECT_EQ(header.size, 0);
}

// TEST_F(DispatcherTest,
//        should_send_disconnect_when_single_chunk_response_is_received) {
//   std::vector<std::uint8_t> capturedHeader;
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//       .WillOnce(CaptureSentBytes(capturedHeader));
//   EXPECT_CALL(mockSocket.get(), send(_, Ne(LPTF_HEADER_SIZE))).Times(0);

//   Dispatcher dispatcher;
//   // total_chunks=1, chunk_index=0 → only chunk = last chunk
//   dispatcher.dispatch(
//       agentSession,
//       makeFrame(MessageType::RESPONSE, makeResponsePayload(0, "result", 1,
//       0)));

//   ASSERT_EQ(capturedHeader.size(), LPTF_HEADER_SIZE);
//   EXPECT_EQ(capturedHeader[5],
//             static_cast<std::uint8_t>(MessageType::DISCONNECT));
// }

// // ─────────────────────────────────────────────────────────────
// // Dispatch — Protocol Application
// // ─────────────────────────────────────────────────────────────

// // The agent should never send DISCONNECT (it's the server that
// // initiates it). If it happens, the server must reject the message
// // with ERROR — that's the correct behavior.
// TEST_F(DispatcherTest,
//        should_send_error_when_agent_violates_protocol_by_sending_disconnect)
//        {
//   std::vector<std::uint8_t> capturedHeader;
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   {
//     InSequence orderedSends;
//     EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//         .WillOnce(CaptureSentBytes(capturedHeader));
//     EXPECT_CALL(mockSocket.get(),
//                 send(_, Gt(std::size_t{0})))  // payload of the error
//         .WillOnce(ReturnAllBytes());
//   }

//   Dispatcher dispatcher;
//   dispatcher.dispatch(agentSession, makeFrame(MessageType::DISCONNECT));

//   ASSERT_EQ(capturedHeader.size(), LPTF_HEADER_SIZE);
//   EXPECT_EQ(capturedHeader[5],
//   static_cast<std::uint8_t>(MessageType::ERROR));
// }

// TEST_F(DispatcherTest,
//        should_send_error_when_server_receives_command_type_message) {
//   // COMMAND is a server → agent message. Receiving it is a protocol
//   violation. std::vector<std::uint8_t> capturedHeader; auto [agentSession,
//   mockSocket] = makeAgentSessionWithMock();
//   {
//     // Sequence ensures the header send is checked before any payload
//     // send for all EXPECT_CALLs in this block.
//     InSequence orderedSends;
//     EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//         .WillOnce(CaptureSentBytes(capturedHeader));
//     EXPECT_CALL(mockSocket.get(), send(_, Gt(std::size_t{0})))
//         .WillOnce(ReturnAllBytes());
//   }

//   Dispatcher dispatcher;
//   dispatcher.dispatch(agentSession, makeFrame(MessageType::COMMAND));

//   ASSERT_EQ(capturedHeader.size(), LPTF_HEADER_SIZE);
//   EXPECT_EQ(capturedHeader[5],
//   static_cast<std::uint8_t>(MessageType::ERROR));
// }

// // ─────────────────────────────────────────────────────────────
// // nextId — monotonie et overflow
// // ─────────────────────────────────────────────────────────────

// TEST_F(DispatcherTest,
//        should_assign_monotonically_increasing_ids_to_successive_commands) {
//   // Each REGISTER triggers a sendCommand(OS_INFO).
//   // We capture the id field (first 2 bytes of the COMMAND payload).
//   std::vector<std::uint16_t> sentCommandIds;
//   Dispatcher dispatcher;

//   for (int i = 0; i < 3; ++i) {
//     auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//     int sendCallCount = 0;

//     EXPECT_CALL(mockSocket.get(), send(_, _))
//         .Times(AtLeast(2))  // header + payload
//         .WillRepeatedly([&](const std::uint8_t* data, std::size_t byteCount)
//         {
//           ++sendCallCount;
//           // Odd calls (1, 3, 5, ...) are COMMAND payloads; id is in first 2
//           // bytes Structure: [id_hi][id_lo][type]...
//           if (sendCallCount % 2 == 0 && byteCount >= 2) {
//             std::vector<std::uint8_t> dataVector(data, data + byteCount);
//             sentCommandIds.push_back(ConvertEndian::readU16BE(dataVector,
//             0));
//           }
//           return SocketResult{static_cast<int>(byteCount), SocketStatus::OK};
//         });

//     dispatcher.dispatch(agentSession, makeFrame(MessageType::REGISTER,
//                                                 makeRegisterPayload("h")));
//   }

//   ASSERT_EQ(sentCommandIds.size(), std::size_t(3));
//   EXPECT_EQ(sentCommandIds[0], 0);
//   EXPECT_EQ(sentCommandIds[1], 1);
//   EXPECT_EQ(sentCommandIds[2], 2);
// }

// TEST_F(DispatcherTest, should_wrap_command_id_to_zero_after_uint16_max) {
//   // nextId() returns a uint16_t. Wrapping from 65536 → 0 is defined
//   // behavior (unsigned overflow). This test documents that we don't crash
//   // and that the id wraps correctly.

//   Dispatcher dispatcher;
//   // Consume the whole uint16_t interval with nextId()
//   for (int i = 0; i <= 65535; ++i) {
//     (void)dispatcher.nextId();
//   }

//   // Next call should return 0.
//   const std::uint16_t wrappedId = dispatcher.nextId();
//   EXPECT_EQ(wrappedId, 0);
// }
