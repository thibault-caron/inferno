#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_test_helpers.hpp"

namespace {

std::vector<uint8_t> makeResponsePayload(
    uint16_t id, uint8_t rawStatus, uint8_t totalChunks, uint8_t chunkIndex,
    uint16_t declaredLen, const std::vector<uint8_t>& dataBytes) {
  std::vector<uint8_t> out;
  out.resize(RESPONSE_FIXED_BYTES + dataBytes.size());
  ConvertEndian::writeU16BE(out, 0, id);
  out[2] = rawStatus;
  out[3] = totalChunks;
  out[4] = chunkIndex;
  ConvertEndian::writeU16BE(out, 5, declaredLen);
  for (std::size_t i = 0; i < dataBytes.size(); ++i) {
    out[RESPONSE_FIXED_BYTES + i] = dataBytes[i];
  }
  return out;
}

}  // namespace

TEST(ProtocolParserResponse, should_parse_response_when_input_is_valid) {
  const std::string data = "ok";
  const std::vector<uint8_t> input = makeResponsePayload(
      9, static_cast<uint8_t>(ResponseStatus::OK), 1, 0,
      static_cast<uint16_t>(data.size()), TestHelpers::bytesFromString(data));

  const ResponsePayload result = ProtocolParser::parseResponsePayload(input);

  EXPECT_EQ(result.id, 9);
  EXPECT_EQ(result.status, ResponseStatus::OK);
  EXPECT_EQ(result.total_chunks, 1);
  EXPECT_EQ(result.chunk_index, 0);
  EXPECT_EQ(result.data, data);
}

TEST(ProtocolParserResponse,
     should_reject_payload_shorter_than_fixed_response_fields) {
  const std::vector<uint8_t> input = {0x00, 0x09, 0x00, 0x01, 0x00, 0x00};

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidSize);
}

TEST(ProtocolParserResponse, should_reject_unknown_status_value) {
  const std::vector<uint8_t> input =
      makeResponsePayload(9, TestHelpers::INVALID_ENUM_VALUE, 1, 0, 0, {});

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidFieldValue);
}

TEST(ProtocolParserResponse, should_reject_zero_total_chunks) {
  const std::vector<uint8_t> input = makeResponsePayload(
      9, static_cast<uint8_t>(ResponseStatus::OK), 0, 0, 0, {});

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidFieldValue);
}

TEST(ProtocolParserResponse, should_reject_chunk_index_out_of_range) {
  const std::vector<uint8_t> input = makeResponsePayload(
      9, static_cast<uint8_t>(ResponseStatus::OK), 2, 2, 0, {});

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidFieldValue);
}

TEST(ProtocolParserResponse,
     should_reject_when_declared_data_length_mismatches_payload_size) {
  const std::vector<uint8_t> input =
      makeResponsePayload(9, static_cast<uint8_t>(ResponseStatus::OK), 1, 0, 10,
                          TestHelpers::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseResponsePayload(input), InvalidSize);
}
