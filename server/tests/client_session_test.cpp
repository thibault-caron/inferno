#include "client_session.hpp"

#include <gtest/gtest.h>

#include "helpers_test.hpp"

// ─────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────

// Feed raw bytes into a ClientSession's buffer.
static void feed(ClientSession& clientSession,
                 const std::vector<std::uint8_t>& bytes) {
  clientSession.buffer.insert(clientSession.buffer.end(), bytes.begin(),
                              bytes.end());
}

// ─────────────────────────────────────────────────────────────
// Empty / incomplete buffer
// ─────────────────────────────────────────────────────────────

TEST(ClientSessionBuffer, should_return_nullopt_when_buffer_is_empty) {
  ClientSession clientSession;
  EXPECT_FALSE(clientSession.tryExtractFrame().has_value());
}

TEST(ClientSessionFrameExtraction,
     should_return_nullopt_and_leave_buffer_intact_when_header_is_incomplete) {
  ClientSession clientSession;
  // 7 bytes - missing the low byte of the size field
  const std::vector<std::uint8_t> partial(
      {'L', 'P', 'T', 'F', LPTF_VERSION,
       static_cast<std::uint8_t>(MessageType::DISCONNECT), 0x00});
  feed(clientSession, partial);

  EXPECT_FALSE(clientSession.tryExtractFrame().has_value());
  // no bytes should be consumed from the buffer since the header is incomplete
  EXPECT_EQ(clientSession.buffer.size(), partial.size());
}

TEST(ClientSessionBuffer,
     should_return_nullopt_when_header_is_complete_but_payload_is_partial) {
  ClientSession clientSession;
  // Header declares a 4-byte payload; only 2 bytes of it arrive.
  auto frame = makeRawFrame(MessageType::REGISTER, {0x01, 0x02, 0x03, 0x04});
  const std::size_t partial = LPTF_HEADER_SIZE + 2;  // header + 2/4 bytes
  feed(clientSession, {frame.begin(), frame.begin() + partial});
  EXPECT_FALSE(clientSession.tryExtractFrame().has_value());
}

// ─────────────────────────────────────────────────────────────
// Complete frame in one shot
// ─────────────────────────────────────────────────────────────

TEST(ClientSessionBuffer,
     should_extract_frame_when_complete_frame_arrives_at_once) {
  ClientSession clientSession;
  const auto payload = makeRegisterPayload("server42");
  feed(clientSession, makeRawFrame(MessageType::REGISTER, payload));

  const auto frame = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::REGISTER);
  EXPECT_EQ(frame->payload, payload);
}

TEST(ClientSessionBuffer,
     should_extract_frame_with_zero_payload_when_disconnect_arrives) {
  ClientSession clientSession;
  feed(clientSession, makeRawFrame(MessageType::DISCONNECT));

  const auto frame = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::DISCONNECT);
  EXPECT_EQ(frame->header.size, 0);
  EXPECT_TRUE(frame->payload.empty());
}

// ─────────────────────────────────────────────────────────────
// Buffer consumption — no data loss, no data duplication
// ─────────────────────────────────────────────────────────────

TEST(ClientSessionFrameExtraction,
     should_return_payload_bytes_identical_to_what_was_sent) {
  ClientSession clientSession;
  // Bytes distincts pour détecter toute corruption ou décalage
  const std::vector<std::uint8_t> payload = {0x00, 0x01, 0x7F, 0x80,
                                             0xFF, 0xAB, 0xCD, 0xEF};
  feed(clientSession, makeRawFrame(MessageType::DATA, payload));

  const auto frame = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->payload, payload);
}

TEST(ClientSessionBuffer,
     should_consume_header_and_payload_bytes_after_extraction) {
  ClientSession clientSession;
  const auto payload = makeRegisterPayload("host");
  const auto raw = makeRawFrame(MessageType::REGISTER, payload);
  feed(clientSession, raw);

  clientSession.tryExtractFrame();  // consume the frame

  EXPECT_TRUE(clientSession.buffer.empty())
      << "Expected buffer to be empty after extracting the only frame, "
      << "but " << clientSession.buffer.size() << " bytes remain";
}

TEST(ClientSessionBuffer,
     should_leave_trailing_bytes_intact_after_extracting_first_frame) {
  ClientSession clientSession;
  // Two frames concatenated — second is partial (only its header)
  const auto frame1 = makeRawFrame(MessageType::DISCONNECT);
  const auto frame2 =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("box"));
  const auto frame2Header = std::vector<std::uint8_t>(
      frame2.begin(), frame2.begin() + LPTF_HEADER_SIZE);

  feed(clientSession, frame1);
  feed(clientSession, frame2Header);  // append incomplete second frame

  const auto extracted = clientSession.tryExtractFrame();
  ASSERT_TRUE(extracted.has_value());
  EXPECT_EQ(extracted->header.type, MessageType::DISCONNECT);

  // The partial second frame must still be in the buffer
  EXPECT_EQ(clientSession.buffer.size(), LPTF_HEADER_SIZE);
}

// ─────────────────────────────────────────────────────────────
// Fragmented delivery (TCP stream reassembly)
// ─────────────────────────────────────────────────────────────

// every byte delivered in a different recv call — should not extract until the
// last byte completes the frame
TEST(
    ClientSessionReassembly,
    should_return_nullopt_for_every_byte_until_the_last_one_completes_the_frame) {
  ClientSession clientSession;
  const auto payload = makeRegisterPayload("fragmented-host");
  const auto rawFrame = makeRawFrame(MessageType::REGISTER, payload);

  // Deliver bytes one at a time
  for (std::size_t i = 0; i + 1 < rawFrame.size(); ++i) {
    clientSession.buffer.push_back(rawFrame[i]);
    EXPECT_FALSE(clientSession.tryExtractFrame().has_value())
        << "Should not extract frame after only " << (i + 1) << " bytes";
  }
  // Last byte
  clientSession.buffer.push_back(rawFrame.back());
  EXPECT_TRUE(clientSession.tryExtractFrame().has_value());
}

TEST(
    ClientSessionBuffer,
    should_extract_frame_when_header_and_payload_arrive_in_separate_recv_calls) {
  ClientSession clientSession;
  const auto payload = makeRegisterPayload("split");
  const auto rawFrame = makeRawFrame(MessageType::REGISTER, payload);

  // Chunk 1: just the header
  feed(clientSession, {rawFrame.begin(), rawFrame.begin() + LPTF_HEADER_SIZE});
  EXPECT_FALSE(clientSession.tryExtractFrame().has_value());

  // Chunk 2: the payload
  feed(clientSession, {rawFrame.begin() + LPTF_HEADER_SIZE, rawFrame.end()});
  const auto frame = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->payload, payload);
}

// ─────────────────────────────────────────────────────────────
// Two consecutive complete frames
// ─────────────────────────────────────────────────────────────

// TCP can send multiple frames' worth of data in a single recv call — should
// extract both frames without losing or corrupting data
TEST(ClientSessionConsecutiveFrames,
     should_extract_first_frame_and_leave_second_frame_header_in_state) {
  ClientSession clientSession;
  const auto payload2 = makeRegisterPayload("second");
  const auto rawFrame2 = makeRawFrame(MessageType::REGISTER, payload2);

  // Frame 1 + only header of Frame 2
  feed(clientSession, makeRawFrame(MessageType::DISCONNECT));
  feed(clientSession,
       {rawFrame2.begin(), rawFrame2.begin() + LPTF_HEADER_SIZE});

  const auto frame1 = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame1.has_value());
  EXPECT_EQ(frame1->header.type, MessageType::DISCONNECT);

  // With an incomplete second frame, header bytes remain buffered.
  EXPECT_EQ(clientSession.buffer.size(), LPTF_HEADER_SIZE);
  EXPECT_FALSE(clientSession.tryExtractFrame().has_value());
}

TEST(ClientSessionBuffer,
     should_extract_two_consecutive_frames_without_data_loss) {
  ClientSession clientSession;
  const auto payload1 = makeRegisterPayload("host1");
  const auto payload2 = makeResponsePayload(0, "result");

  feed(clientSession, makeRawFrame(MessageType::REGISTER, payload1));
  feed(clientSession, makeRawFrame(MessageType::RESPONSE, payload2));

  const auto frame1 = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame1.has_value());
  EXPECT_EQ(frame1->header.type, MessageType::REGISTER);
  EXPECT_EQ(frame1->payload, payload1);

  const auto frame2 = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame2.has_value());
  EXPECT_EQ(frame2->header.type, MessageType::RESPONSE);
  EXPECT_EQ(frame2->payload, payload2);

  // Buffer must be empty after both frames extracted
  EXPECT_TRUE(clientSession.buffer.empty());
}

TEST(ClientSessionBuffer,
     should_extract_second_frame_only_after_it_becomes_complete) {
  ClientSession clientSession;
  const auto payload2 = makeRegisterPayload("late");
  const auto raw2 = makeRawFrame(MessageType::REGISTER, payload2);

  // Feed first full frame + only header of second
  feed(clientSession, makeRawFrame(MessageType::DISCONNECT));
  feed(clientSession, {raw2.begin(), raw2.begin() + LPTF_HEADER_SIZE});

  ASSERT_TRUE(clientSession.tryExtractFrame().has_value());  // frame 1 out
  EXPECT_FALSE(
      clientSession.tryExtractFrame().has_value());  // frame 2 still partial

  // Complete second frame
  feed(clientSession, {raw2.begin() + LPTF_HEADER_SIZE, raw2.end()});
  const auto frame2 = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame2.has_value());
  EXPECT_EQ(frame2->payload, payload2);
}

// ─────────────────────────────────────────────────────────────
// Header fields are decoded correctly
// ─────────────────────────────────────────────────────────────

TEST(ClientSessionHeaderDecoding,
     should_decode_header_version_and_size_correctly) {
  ClientSession clientSession;
  const auto payload = std::vector<std::uint8_t>(42, 0xAB);
  feed(clientSession, makeRawFrame(MessageType::COMMAND, payload));

  const auto frame = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.version, LPTF_VERSION);
  EXPECT_EQ(frame->header.size, 42);
  EXPECT_EQ(frame->header.type, MessageType::COMMAND);
}

TEST(ClientSessionHeaderDecoding,
     should_decode_big_endian_16bit_size_field_correctly) {
  ClientSession clientSession;
  // size = 0x01FF = 511 bytes
  const auto payload = std::vector<std::uint8_t>(511, 0x00);
  feed(clientSession, makeRawFrame(MessageType::DATA, payload));

  const auto frame = clientSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.size, 511);
  EXPECT_EQ(frame->payload.size(), std::size_t(511));
}
