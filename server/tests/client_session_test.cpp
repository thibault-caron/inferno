#include <gtest/gtest.h>

#include "agent_session.hpp"
#include "helpers_test.hpp"

// ─────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────

// Feed raw bytes into a AgentSession's buffer.
static void feed(AgentSession& agentSession,
                 const std::vector<std::uint8_t>& bytes) {
  agentSession.buffer.insert(agentSession.buffer.end(), bytes.begin(),
                             bytes.end());
}

// ─────────────────────────────────────────────────────────────
// Empty / incomplete buffer
// ─────────────────────────────────────────────────────────────

TEST(AgentSessionBuffer, should_return_nullopt_when_buffer_is_empty) {
  AgentSession agentSession;
  EXPECT_FALSE(agentSession.tryExtractFrame().has_value());
}

TEST(AgentSessionFrameExtraction,
     should_return_nullopt_and_leave_buffer_intact_when_header_is_incomplete) {
  AgentSession agentSession;
  // 7 bytes - missing the low byte of the size field
  const std::vector<std::uint8_t> partial(
      {'L', 'P', 'T', 'F', LPTF_VERSION,
       static_cast<std::uint8_t>(MessageType::DISCONNECT), 0x00});
  feed(agentSession, partial);

  EXPECT_FALSE(agentSession.tryExtractFrame().has_value());
  // no bytes should be consumed from the buffer since the header is incomplete
  EXPECT_EQ(agentSession.buffer.size(), partial.size());
}

TEST(AgentSessionBuffer,
     should_return_nullopt_when_header_is_complete_but_payload_is_partial) {
  AgentSession agentSession;
  // Header declares a 4-byte payload; only 2 bytes of it arrive.
  const std::vector<std::uint8_t> frame =
      makeRawFrame(MessageType::REGISTER, {0x01, 0x02, 0x03, 0x04});
  const std::size_t partial = LPTF_HEADER_SIZE + 2;  // header + 2/4 bytes
  feed(agentSession, {frame.begin(), frame.begin() + partial});
  EXPECT_FALSE(agentSession.tryExtractFrame().has_value());
}

// ─────────────────────────────────────────────────────────────
// Complete frame in one shot
// ─────────────────────────────────────────────────────────────

TEST(AgentSessionBuffer,
     should_extract_frame_when_complete_frame_arrives_at_once) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload = makeRegisterPayload("server42");
  feed(agentSession, makeRawFrame(MessageType::REGISTER, payload));

  const std::optional<Frame> frame = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::REGISTER);
  EXPECT_EQ(frame->payload, payload);
}

TEST(AgentSessionBuffer,
     should_extract_frame_with_zero_payload_when_disconnect_arrives) {
  AgentSession agentSession;
  feed(agentSession, makeRawFrame(MessageType::DISCONNECT));

  const std::optional<Frame> frame = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::DISCONNECT);
  EXPECT_EQ(frame->header.size, 0);
  EXPECT_TRUE(frame->payload.empty());
}

// ─────────────────────────────────────────────────────────────
// Buffer consumption — no data loss, no data duplication
// ─────────────────────────────────────────────────────────────

TEST(AgentSessionFrameExtraction,
     should_return_payload_bytes_identical_to_what_was_sent) {
  AgentSession agentSession;
  // Bytes distincts pour détecter toute corruption ou décalage
  const std::vector<std::uint8_t> payload = {0x00, 0x01, 0x7F, 0x80,
                                             0xFF, 0xAB, 0xCD, 0xEF};
  feed(agentSession, makeRawFrame(MessageType::DATA, payload));

  const std::optional<Frame> frame = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->payload, payload);
}

TEST(AgentSessionBuffer,
     should_consume_header_and_payload_bytes_after_extraction) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload = makeRegisterPayload("host");
  const std::vector<std::uint8_t> raw =
      makeRawFrame(MessageType::REGISTER, payload);
  feed(agentSession, raw);

  agentSession.tryExtractFrame();  // consume the frame

  EXPECT_TRUE(agentSession.buffer.empty())
      << "Expected buffer to be empty after extracting the only frame, "
      << "but " << agentSession.buffer.size() << " bytes remain";
}

TEST(AgentSessionBuffer,
     should_leave_trailing_bytes_intact_after_extracting_first_frame) {
  AgentSession agentSession;
  // Two frames concatenated — second is partial (only its header)
  const std::vector<std::uint8_t> frame1 =
      makeRawFrame(MessageType::DISCONNECT);
  const std::vector<std::uint8_t> frame2 =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("box"));
  const std::vector<std::uint8_t> frame2Header = std::vector<std::uint8_t>(
      frame2.begin(), frame2.begin() + LPTF_HEADER_SIZE);

  feed(agentSession, frame1);
  feed(agentSession, frame2Header);  // append incomplete second frame

  const std::optional<Frame> extracted = agentSession.tryExtractFrame();
  ASSERT_TRUE(extracted.has_value());
  EXPECT_EQ(extracted->header.type, MessageType::DISCONNECT);

  // The partial second frame must still be in the buffer
  EXPECT_EQ(agentSession.buffer.size(), LPTF_HEADER_SIZE);
}

// ─────────────────────────────────────────────────────────────
// Fragmented delivery (TCP stream reassembly)
// ─────────────────────────────────────────────────────────────

// every byte delivered in a different recv call — should not extract until the
// last byte completes the frame
TEST(
    AgentSessionReassembly,
    should_return_nullopt_for_every_byte_until_the_last_one_completes_the_frame) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload =
      makeRegisterPayload("fragmented-host");
  const std::vector<std::uint8_t> rawFrame =
      makeRawFrame(MessageType::REGISTER, payload);

  // Deliver bytes one at a time
  for (std::size_t i = 0; i + 1 < rawFrame.size(); ++i) {
    agentSession.buffer.push_back(rawFrame[i]);
    EXPECT_FALSE(agentSession.tryExtractFrame().has_value())
        << "Should not extract frame after only " << (i + 1) << " bytes";
  }
  // Last byte
  agentSession.buffer.push_back(rawFrame.back());
  EXPECT_TRUE(agentSession.tryExtractFrame().has_value());
}

TEST(
    AgentSessionBuffer,
    should_extract_frame_when_header_and_payload_arrive_in_separate_recv_calls) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload = makeRegisterPayload("split");
  const std::vector<std::uint8_t> rawFrame =
      makeRawFrame(MessageType::REGISTER, payload);

  // Chunk 1: just the header
  feed(agentSession, {rawFrame.begin(), rawFrame.begin() + LPTF_HEADER_SIZE});
  EXPECT_FALSE(agentSession.tryExtractFrame().has_value());

  // Chunk 2: the payload
  feed(agentSession, {rawFrame.begin() + LPTF_HEADER_SIZE, rawFrame.end()});
  const std::optional<Frame> frame = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->payload, payload);
}

// ─────────────────────────────────────────────────────────────
// Two consecutive complete frames
// ─────────────────────────────────────────────────────────────

// TCP can send multiple frames' worth of data in a single recv call — should
// extract both frames without losing or corrupting data
TEST(AgentSessionConsecutiveFrames,
     should_extract_first_frame_and_leave_second_frame_header_in_state) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload2 = makeRegisterPayload("second");
  const std::vector<std::uint8_t> rawFrame2 =
      makeRawFrame(MessageType::REGISTER, payload2);

  // Frame 1 + only header of Frame 2
  feed(agentSession, makeRawFrame(MessageType::DISCONNECT));
  feed(agentSession, {rawFrame2.begin(), rawFrame2.begin() + LPTF_HEADER_SIZE});

  const std::optional<Frame> frame1 = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame1.has_value());
  EXPECT_EQ(frame1->header.type, MessageType::DISCONNECT);

  // With an incomplete second frame, header bytes remain buffered.
  EXPECT_EQ(agentSession.buffer.size(), LPTF_HEADER_SIZE);
  EXPECT_FALSE(agentSession.tryExtractFrame().has_value());
}

TEST(AgentSessionBuffer,
     should_extract_two_consecutive_frames_without_data_loss) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload1 = makeRegisterPayload("host1");
  const std::vector<std::uint8_t> payload2 = makeResponsePayload(0, "result");

  feed(agentSession, makeRawFrame(MessageType::REGISTER, payload1));
  feed(agentSession, makeRawFrame(MessageType::RESPONSE, payload2));

  const std::optional<Frame> frame1 = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame1.has_value());
  EXPECT_EQ(frame1->header.type, MessageType::REGISTER);
  EXPECT_EQ(frame1->payload, payload1);

  const std::optional<Frame> frame2 = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame2.has_value());
  EXPECT_EQ(frame2->header.type, MessageType::RESPONSE);
  EXPECT_EQ(frame2->payload, payload2);

  // Buffer must be empty after both frames extracted
  EXPECT_TRUE(agentSession.buffer.empty());
}

TEST(AgentSessionBuffer,
     should_extract_second_frame_only_after_it_becomes_complete) {
  AgentSession agentSession;
  const std::vector<std::uint8_t> payload2 = makeRegisterPayload("late");
  const std::vector<std::uint8_t> raw2 =
      makeRawFrame(MessageType::REGISTER, payload2);

  // Feed first full frame + only header of second
  feed(agentSession, makeRawFrame(MessageType::DISCONNECT));
  feed(agentSession, {raw2.begin(), raw2.begin() + LPTF_HEADER_SIZE});

  ASSERT_TRUE(agentSession.tryExtractFrame().has_value());  // frame 1 out
  EXPECT_FALSE(
      agentSession.tryExtractFrame().has_value());  // frame 2 still partial

  // Complete second frame
  feed(agentSession, {raw2.begin() + LPTF_HEADER_SIZE, raw2.end()});
  const std::optional<Frame> frame2 = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame2.has_value());
  EXPECT_EQ(frame2->payload, payload2);
}

// ─────────────────────────────────────────────────────────────
// Header fields are decoded correctly
// ─────────────────────────────────────────────────────────────

TEST(AgentSessionHeaderDecoding,
     should_decode_header_version_and_size_correctly) {
  AgentSession agentSession;
  const auto payload = std::vector<std::uint8_t>(42, 0xAB);
  feed(agentSession, makeRawFrame(MessageType::COMMAND, payload));

  const auto frame = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.version, LPTF_VERSION);
  EXPECT_EQ(frame->header.size, 42);
  EXPECT_EQ(frame->header.type, MessageType::COMMAND);
}

TEST(AgentSessionHeaderDecoding,
     should_decode_big_endian_16bit_size_field_correctly) {
  AgentSession agentSession;
  // size = 0x01FF = 511 bytes
  const auto payload = std::vector<std::uint8_t>(511, 0x00);
  feed(agentSession, makeRawFrame(MessageType::DATA, payload));

  const auto frame = agentSession.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.size, 511);
  EXPECT_EQ(frame->payload.size(), std::size_t(511));
}
