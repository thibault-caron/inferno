#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "c1_test_helpers.hpp"
#include "client_session.hpp"
#include "dispatcher.hpp"
#include "mock_socket.hpp"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Gt;
using ::testing::InSequence;
using ::testing::Ne;
using ::testing::Return;

// ── Fixture ──────────────────────────────────────────────────
class DispatcherTest : public ::testing::Test {
 protected:
  void SetUp() override {
    txSocket = std::make_unique<MockSocket>();
    ON_CALL(*txSocket, isValid()).WillByDefault(Return(true));
    ON_CALL(*txSocket, send(_, _)).WillByDefault(ReturnAllBytes());
  }

  // Constructs a Frame from a MessageType and a serialized payload.
  static Frame makeFrame(MessageType type,
                         const std::vector<std::uint8_t>& payload = {}) {
    LptfHeader hdr{{'L', 'P', 'T', 'F'},
                   LPTF_VERSION,
                   type,
                   static_cast<std::uint16_t>(payload.size())};
    return Frame{hdr, payload};
  }

  std::unique_ptr<MockSocket> txSocket;
};

// ─────────────────────────────────────────────────────────────
// onRegister
// ─────────────────────────────────────────────────────────────

TEST_F(DispatcherTest,
       should_store_hostname_in_client_session_when_register_is_received) {
  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);

  dispatcher.dispatch(
      cs, makeFrame(MessageType::REGISTER, makeRegisterPayload("worker-42")));

  EXPECT_EQ(cs.getClientInfo().hostname, "worker-42");
}

TEST_F(
    DispatcherTest,
    should_store_os_type_and_arch_in_client_session_when_register_is_received) {
  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);

  dispatcher.dispatch(cs, makeFrame(MessageType::REGISTER,
                                    makeRegisterPayload("host", OSType::WINDOWS,
                                                        ArchType::X64)));

  EXPECT_EQ(cs.getClientInfo().os_type, OSType::WINDOWS);
  EXPECT_EQ(cs.getClientInfo().arch, ArchType::X64);
}

TEST_F(DispatcherTest, should_send_command_message_immediately_after_register) {
  // Verify byte 5 of the serialized header = MessageType::COMMAND
  std::vector<std::uint8_t> capturedHdr;
  {
    InSequence seq;
    EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
        .WillOnce([&](const std::uint8_t* d, std::size_t n) {
          capturedHdr.assign(d, d + n);
          return SocketResult{static_cast<int>(n), SocketStatus::OK};
        });
    EXPECT_CALL(*txSocket,
                send(_, Gt(std::size_t{0})))  // payload of the command
        .WillOnce(ReturnAllBytes());
  }

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  dispatcher.dispatch(
      cs, makeFrame(MessageType::REGISTER, makeRegisterPayload("h")));

  ASSERT_EQ(capturedHdr.size(), LPTF_HEADER_SIZE);
  EXPECT_EQ(capturedHdr[5], static_cast<std::uint8_t>(MessageType::COMMAND));
}

TEST_F(DispatcherTest, should_send_os_info_command_type_after_register) {
  // Byte 2 of a COMMAND payload = CommandType
  std::vector<std::uint8_t> capturedPayload;
  {
    InSequence seq;
    EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
        .WillOnce(ReturnAllBytes());
    EXPECT_CALL(*txSocket, send(_, Gt(std::size_t{0})))
        .WillOnce([&](const std::uint8_t* d, std::size_t n) {
          capturedPayload.assign(d, d + n);
          return SocketResult{static_cast<int>(n), SocketStatus::OK};
        });
  }

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  dispatcher.dispatch(
      cs, makeFrame(MessageType::REGISTER, makeRegisterPayload("h")));

  // Structure CommandPayload: [id_hi][id_lo][type][data_len_hi][data_len_lo]
  ASSERT_GE(capturedPayload.size(), std::size_t(3));
  EXPECT_EQ(capturedPayload[2],
            static_cast<std::uint8_t>(CommandType::OS_INFO));
}

TEST_F(DispatcherTest,
       should_throw_when_header_send_is_shorter_than_serialized_header) {
  EXPECT_CALL(*txSocket, send(_, _))
      .WillOnce(Return(SocketResult{LPTF_HEADER_SIZE - 1, SocketStatus::OK}));

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);

  EXPECT_THROW(dispatcher.dispatch(cs, makeFrame(MessageType::REGISTER,
                                                 makeRegisterPayload("h"))),
               std::runtime_error);
}

TEST_F(DispatcherTest,
       should_throw_when_payload_send_is_shorter_than_expected) {
  {
    InSequence seq;
    EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
        .WillOnce([](const std::uint8_t*, std::size_t len) {
          return SocketResult{static_cast<int>(len), SocketStatus::OK};
        });
    EXPECT_CALL(*txSocket, send(_, Gt(std::size_t{0})))
        .WillOnce(Return(SocketResult{1, SocketStatus::OK}));
  }

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);

  EXPECT_THROW(dispatcher.dispatch(cs, makeFrame(MessageType::REGISTER,
                                                 makeRegisterPayload("h"))),
               std::runtime_error);
}

// ─────────────────────────────────────────────────────────────
// onResponse — chunk handling
// ─────────────────────────────────────────────────────────────

TEST_F(DispatcherTest,
       should_not_send_anything_when_response_chunk_is_not_the_last_one) {
  // chunk_index=0, total_chunks=3 → not the last chunk, so no send expected.
  EXPECT_CALL(*txSocket, send(_, _)).Times(0);

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  dispatcher.dispatch(cs, makeFrame(MessageType::RESPONSE,
                                    makeResponsePayload(0, "partial", 3, 0)));
}

TEST_F(DispatcherTest,
       should_not_send_anything_when_intermediate_chunk_arrives) {
  // chunk_index=1, total_chunks=3 → middle chunk: no send expected.
  EXPECT_CALL(*txSocket, send(_, _)).Times(0);

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  dispatcher.dispatch(cs, makeFrame(MessageType::RESPONSE,
                                    makeResponsePayload(0, "middle", 3, 1)));
}

TEST_F(DispatcherTest,
       should_send_disconnect_when_last_response_chunk_is_received) {
  std::vector<std::uint8_t> capturedHdr;
  EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
      .WillOnce([&](const std::uint8_t* d, std::size_t n) {
        capturedHdr.assign(d, d + n);
        return SocketResult{static_cast<int>(n), SocketStatus::OK};
      });
  // DISCONNECT has no payload.
  EXPECT_CALL(*txSocket, send(_, Ne(LPTF_HEADER_SIZE))).Times(0);

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  // chunk_index=2, total_chunks=3 → last chunk
  dispatcher.dispatch(cs, makeFrame(MessageType::RESPONSE,
                                    makeResponsePayload(0, "final", 3, 2)));

  ASSERT_EQ(capturedHdr.size(), LPTF_HEADER_SIZE);
  EXPECT_EQ(capturedHdr[5], static_cast<std::uint8_t>(MessageType::DISCONNECT));
}

TEST_F(DispatcherTest,
       should_send_disconnect_when_single_chunk_response_is_received) {
  std::vector<std::uint8_t> capturedHdr;
  EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
      .WillOnce([&](const std::uint8_t* d, std::size_t n) {
        capturedHdr.assign(d, d + n);
        return SocketResult{static_cast<int>(n), SocketStatus::OK};
      });
  EXPECT_CALL(*txSocket, send(_, Ne(LPTF_HEADER_SIZE))).Times(0);

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  // total_chunks=1, chunk_index=0 → only chunk = last chunk
  dispatcher.dispatch(cs, makeFrame(MessageType::RESPONSE,
                                    makeResponsePayload(0, "result", 1, 0)));

  ASSERT_EQ(capturedHdr.size(), LPTF_HEADER_SIZE);
  EXPECT_EQ(capturedHdr[5], static_cast<std::uint8_t>(MessageType::DISCONNECT));
}

// ─────────────────────────────────────────────────────────────
// Dispatch — Protocol Application
// ─────────────────────────────────────────────────────────────

// The client should never send DISCONNECT (it's the server that
// initiates it). If it happens, the server must reject the message
// with ERROR — that's the correct behavior.
TEST_F(DispatcherTest,
       should_send_error_when_client_violates_protocol_by_sending_disconnect) {
  std::vector<std::uint8_t> capturedHdr;
  {
    InSequence seq;
    EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
        .WillOnce([&](const std::uint8_t* d, std::size_t n) {
          capturedHdr.assign(d, d + n);
          return SocketResult{static_cast<int>(n), SocketStatus::OK};
        });
    EXPECT_CALL(*txSocket, send(_, Gt(std::size_t{0})))  // payload of the error
        .WillOnce(ReturnAllBytes());
  }

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  dispatcher.dispatch(cs, makeFrame(MessageType::DISCONNECT));

  ASSERT_EQ(capturedHdr.size(), LPTF_HEADER_SIZE);
  EXPECT_EQ(capturedHdr[5], static_cast<std::uint8_t>(MessageType::ERROR));
}

TEST_F(DispatcherTest,
       should_send_error_when_server_receives_command_type_message) {
  // COMMAND is a server → client message. Receiving it is a protocol violation.
  std::vector<std::uint8_t> capturedHdr;
  {
    // Sequence ensures the header send is checked before any payload
    // send for all EXPECT_CALLs in this block.
    InSequence sequence;
    EXPECT_CALL(*txSocket, send(_, LPTF_HEADER_SIZE))
        .WillOnce([&](const std::uint8_t* d, std::size_t n) {
          capturedHdr.assign(d, d + n);
          return SocketResult{static_cast<int>(n), SocketStatus::OK};
        });
    EXPECT_CALL(*txSocket, send(_, Gt(std::size_t{0})))
        .WillOnce(ReturnAllBytes());
  }

  Dispatcher dispatcher(*txSocket);
  ClientSession cs(nullptr);
  dispatcher.dispatch(cs, makeFrame(MessageType::COMMAND));

  ASSERT_EQ(capturedHdr.size(), LPTF_HEADER_SIZE);
  EXPECT_EQ(capturedHdr[5], static_cast<std::uint8_t>(MessageType::ERROR));
}

// ─────────────────────────────────────────────────────────────
// nextId — monotonie et overflow
// ─────────────────────────────────────────────────────────────

TEST_F(DispatcherTest,
       should_assign_monotonically_increasing_ids_to_successive_commands) {
  // Each REGISTER triggers a sendCommand(OS_INFO).
  // We capture the id field (first 2 bytes of the COMMAND payload).
  std::vector<std::uint16_t> sentIds;
  int callIndex = 0;

  EXPECT_CALL(*txSocket, send(_, _))
      .Times(AtLeast(4))  // 2 × (header + payload)
      .WillRepeatedly([&](const std::uint8_t* d, std::size_t n) {
        ++callIndex;
        // Even calls are COMMAND payloads; id is in the first 2 bytes
        // (big-endian). Structure: [id_hi][id_lo][type]...
        if (callIndex % 2 == 0 && n >= 2) {
          sentIds.push_back(static_cast<std::uint16_t>((d[0] << 8) | d[1]));
        }
        return SocketResult{static_cast<int>(n), SocketStatus::OK};
      });

  Dispatcher dispatcher(*txSocket);
  for (int i = 0; i < 3; ++i) {
    ClientSession cs(nullptr);
    dispatcher.dispatch(
        cs, makeFrame(MessageType::REGISTER, makeRegisterPayload("h")));
  }

  ASSERT_EQ(sentIds.size(), std::size_t(3));
  EXPECT_EQ(sentIds[0], 0);
  EXPECT_EQ(sentIds[1], 1);
  EXPECT_EQ(sentIds[2], 2);
}

TEST_F(DispatcherTest, should_wrap_command_id_to_zero_after_uint16_max) {
  // nextId() returns a uint16_t. Wrapping from 65536 → 0 is defined
  // behavior (unsigned overflow). This test documents that we don't crash
  // and that the id wraps correctly.

  Dispatcher dispatcher(*txSocket);
  // Consume the whole uint16_t interval with nextId()
  for (int i = 0; i <= 65535; ++i) {
    (void)dispatcher.nextId();
  }

  // Next call should return 0.
  const std::uint16_t wrappedId = dispatcher.nextId();
  EXPECT_EQ(wrappedId, 0);
}
