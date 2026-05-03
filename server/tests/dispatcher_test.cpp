// TODO : re write test for server_dispatcher + create test for IDispatcher in shared
// #include "dispatcher.hpp"

// #include <gmock/gmock.h>
// #include <gtest/gtest.h>

// #include <functional>

// #include "agent_session.hpp"
// #include "convert_endian.hpp"
// #include "helpers_test.hpp"
// #include "mock_socket.hpp"

// // todo:
// // LPTF_HEADER_SIZE type mismatch in matchers
// // cpp// LPTF_HEADER_SIZE is constexpr std::uint8_t
// // // send() second arg is std::size_t
// // // GMock may refuse the implicit conversion with -Wpedantic
// // EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))  // ← type mismatch
// // EXPECT_CALL(mockSocket.get(), send(_, Ne(LPTF_HEADER_SIZE))) // ← same
// // Fix:
// // cppEXPECT_CALL(mockSocket.get(), send(_, static_cast<std::size_t>(LPTF_HEADER_SIZE)))
// // EXPECT_CALL(mockSocket.get(), send(_, Ne(static_cast<std::size_t>(LPTF_HEADER_SIZE))

// namespace {

// // Captures all bytes sent in a mocked send() call and returns a successful
// // "all bytes sent" socket result.
// std::function<SocketResult(const std::uint8_t*, std::size_t)> CaptureSentBytes(
//     std::vector<std::uint8_t>& destination) {
//   return [&destination](const std::uint8_t* data,
//                         std::size_t byteCount) -> SocketResult {
//     destination.assign(data, data + byteCount);
//     return SocketResult{static_cast<int>(byteCount), SocketStatus::OK};
//   };
// }

// }  // namespace

// // Bring commonly used gMock matchers into local scope to keep
// // EXPECT_CALL statements short and focused on behavior.
// using ::testing::_;        // any value in EXPECT_CALL/ON_CALL.
// using ::testing::AtLeast;  // AtLeast(n) = expect at least n calls matching this
//                            // statement.
// using ::testing::Gt;       // Gt(n) = argument greater than n
// using ::testing::InSequence;  // InSequence s; EXPECT_CALL(...)...;
//                               // EXPECT_CALL(...)...; → these calls must happen
//                               // in this order.
// using ::testing::Ne;          // Ne(value) = argument not equal to value
// using ::testing::Return;  // Return(value) = return value from a mocked method
//                           // when called with matching arguments.

// // ── Fixture ──────────────────────────────────────────────────
// class DispatcherTest : public ::testing::Test {
//  protected:
//   using MockedAgentSession =
//       std::pair<AgentSession, std::reference_wrapper<MockSocket>>;

//   // Constructs a Frame from a MessageType and a serialized payload.
//   static Frame makeFrame(MessageType type,
//                          const std::vector<std::uint8_t>& payload = {}) {
//     LptfHeader header{{'L', 'P', 'T', 'F'},
//                       LPTF_VERSION,
//                       type,
//                       static_cast<std::uint16_t>(payload.size())};
//     return Frame{header, payload};
//   }

//   // Helper to create a AgentSession with a MockSocket that we can set
//   // expectations on. Returns a pair: (session, mockSocketRef).
//   // The mock is owned by AgentSession and exposed as a reference for testing.
//   static MockedAgentSession makeAgentSessionWithMock() {
//     std::unique_ptr<MockSocket> mock = std::make_unique<MockSocket>();
//     MockSocket& mockRef = *mock;
//     AgentSession session(std::move(mock));

//     // Set default expectations (can be overridden in tests)
//     ON_CALL(mockRef, isValid()).WillByDefault(Return(true));
//     ON_CALL(mockRef, send(_, _)).WillByDefault(ReturnAllBytes());

//     return {std::move(session), std::ref(mockRef)};
//   }
// };

// // ─────────────────────────────────────────────────────────────
// // onRegister
// // ─────────────────────────────────────────────────────────────

// TEST_F(DispatcherTest,
//        should_store_hostname_in_agent_session_when_register_is_received) {
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   Dispatcher dispatcher;

//   dispatcher.dispatch(
//       agentSession,
//       makeFrame(MessageType::REGISTER, makeRegisterPayload("worker-42")));

//   EXPECT_EQ(agentSession.getAgentInfo().hostname, "worker-42");
// }

// TEST_F(
//     DispatcherTest,
//     should_store_os_type_and_arch_in_agent_session_when_register_is_received) {
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   Dispatcher dispatcher;

//   dispatcher.dispatch(
//       agentSession,
//       makeFrame(MessageType::REGISTER,
//                 makeRegisterPayload("host", OSType::WINDOWS, ArchType::X64)));

//   EXPECT_EQ(agentSession.getAgentInfo().os_type, OSType::WINDOWS);
//   EXPECT_EQ(agentSession.getAgentInfo().arch, ArchType::X64);
// }

// TEST_F(DispatcherTest, should_send_command_message_immediately_after_register) {
//   // Verify byte 5 of the serialized header = MessageType::COMMAND
//   std::vector<std::uint8_t> capturedHeader;
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   {
//     InSequence orderedSends;
//     EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//         .WillOnce(CaptureSentBytes(capturedHeader));
//     EXPECT_CALL(mockSocket.get(),
//                 send(_, Gt(std::size_t{0})))  // payload of the command
//         .WillOnce(ReturnAllBytes());
//   }

//   Dispatcher dispatcher;
//   dispatcher.dispatch(
//       agentSession, makeFrame(MessageType::REGISTER, makeRegisterPayload("h")));

//   ASSERT_EQ(capturedHeader.size(), LPTF_HEADER_SIZE);
//   EXPECT_EQ(capturedHeader[5], static_cast<std::uint8_t>(MessageType::COMMAND));
// }

// TEST_F(DispatcherTest, should_send_os_info_command_type_after_register) {
//   // Byte 2 of a COMMAND payload = CommandType
//   std::vector<std::uint8_t> capturedCommandPayload;
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   {
//     InSequence orderedSends;
//     EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//         .WillOnce(ReturnAllBytes());
//     EXPECT_CALL(mockSocket.get(), send(_, Gt(std::size_t{0})))
//         .WillOnce(CaptureSentBytes(capturedCommandPayload));
//   }

//   Dispatcher dispatcher;
//   dispatcher.dispatch(
//       agentSession, makeFrame(MessageType::REGISTER, makeRegisterPayload("h")));

//   // Structure CommandPayload: [id_hi][id_lo][type][data_len_hi][data_len_lo]
//   ASSERT_GE(capturedCommandPayload.size(), std::size_t(3));
//   EXPECT_EQ(capturedCommandPayload[2],
//             static_cast<std::uint8_t>(CommandType::OS_INFO));
// }

// TEST_F(DispatcherTest,
//        should_throw_when_header_send_is_shorter_than_serialized_header) {
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   EXPECT_CALL(mockSocket.get(), send(_, _))
//       .WillOnce(Return(SocketResult{LPTF_HEADER_SIZE - 1, SocketStatus::OK}));

//   Dispatcher dispatcher;

//   EXPECT_THROW(
//       dispatcher.dispatch(agentSession, makeFrame(MessageType::REGISTER,
//                                                   makeRegisterPayload("h"))),
//       std::runtime_error);
// }

// TEST_F(DispatcherTest,
//        should_throw_when_payload_send_is_shorter_than_expected) {
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   {
//     InSequence orderedSends;
//     EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//         .WillOnce([](const std::uint8_t*, std::size_t byteCount) {
//           return SocketResult{static_cast<int>(byteCount), SocketStatus::OK};
//         });
//     EXPECT_CALL(mockSocket.get(), send(_, Gt(std::size_t{0})))
//         .WillOnce(Return(SocketResult{1, SocketStatus::OK}));
//   }

//   Dispatcher dispatcher;

//   EXPECT_THROW(
//       dispatcher.dispatch(agentSession, makeFrame(MessageType::REGISTER,
//                                                   makeRegisterPayload("h"))),
//       std::runtime_error);
// }

// // ─────────────────────────────────────────────────────────────
// // onResponse — chunk handling
// // ─────────────────────────────────────────────────────────────

// TEST_F(DispatcherTest,
//        should_not_send_anything_when_response_chunk_is_not_the_last_one) {
//   // chunk_index=0, total_chunks=3 → not the last chunk, so no send expected.
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   EXPECT_CALL(mockSocket.get(), send(_, _)).Times(0);

//   Dispatcher dispatcher;
//   dispatcher.dispatch(agentSession,
//                       makeFrame(MessageType::RESPONSE,
//                                 makeResponsePayload(0, "partial", 3, 0)));
// }

// TEST_F(DispatcherTest,
//        should_not_send_anything_when_intermediate_chunk_arrives) {
//   // chunk_index=1, total_chunks=3 → middle chunk: no send expected.
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   EXPECT_CALL(mockSocket.get(), send(_, _)).Times(0);

//   Dispatcher dispatcher;
//   dispatcher.dispatch(
//       agentSession,
//       makeFrame(MessageType::RESPONSE, makeResponsePayload(0, "middle", 3, 1)));
// }

// TEST_F(DispatcherTest,
//        should_send_disconnect_when_last_response_chunk_is_received) {
//   std::vector<std::uint8_t> capturedHeader;
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
//   EXPECT_CALL(mockSocket.get(), send(_, LPTF_HEADER_SIZE))
//       .WillOnce(CaptureSentBytes(capturedHeader));
//   // DISCONNECT has no payload.
//   EXPECT_CALL(mockSocket.get(), send(_, Ne(LPTF_HEADER_SIZE))).Times(0);

//   Dispatcher dispatcher;
//   // chunk_index=2, total_chunks=3 → last chunk
//   dispatcher.dispatch(
//       agentSession,
//       makeFrame(MessageType::RESPONSE, makeResponsePayload(0, "final", 3, 2)));

//   ASSERT_EQ(capturedHeader.size(), LPTF_HEADER_SIZE);
//   EXPECT_EQ(capturedHeader[5],
//             static_cast<std::uint8_t>(MessageType::DISCONNECT));
// }

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
//       makeFrame(MessageType::RESPONSE, makeResponsePayload(0, "result", 1, 0)));

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
//        should_send_error_when_agent_violates_protocol_by_sending_disconnect) {
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
//   EXPECT_EQ(capturedHeader[5], static_cast<std::uint8_t>(MessageType::ERROR));
// }

// TEST_F(DispatcherTest,
//        should_send_error_when_server_receives_command_type_message) {
//   // COMMAND is a server → agent message. Receiving it is a protocol violation.
//   std::vector<std::uint8_t> capturedHeader;
//   auto [agentSession, mockSocket] = makeAgentSessionWithMock();
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
//   EXPECT_EQ(capturedHeader[5], static_cast<std::uint8_t>(MessageType::ERROR));
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
//         .WillRepeatedly([&](const std::uint8_t* data, std::size_t byteCount) {
//           ++sendCallCount;
//           // Odd calls (1, 3, 5, ...) are COMMAND payloads; id is in first 2
//           // bytes Structure: [id_hi][id_lo][type]...
//           if (sendCallCount % 2 == 0 && byteCount >= 2) {
//             std::vector<std::uint8_t> dataVector(data, data + byteCount);
//             sentCommandIds.push_back(ConvertEndian::readU16BE(dataVector, 0));
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
