#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_test_helpers.hpp"

namespace {

std::vector<uint8_t> makeErrorPayload(
    uint8_t rawCode, uint16_t declaredLen,
    const std::vector<uint8_t>& messageBytes) {
  std::vector<uint8_t> out;
  out.reserve(ERROR_FIXED_BYTES + messageBytes.size());
  out.push_back(rawCode);
  TestHelpers::appendU16BE(out, declaredLen);
  out.insert(out.end(), messageBytes.begin(), messageBytes.end());
  return out;
}

}  // namespace

TEST(ProtocolParserError, should_parse_error_payload_when_input_is_valid) {
  const std::string message = "invalid format";
  const std::vector<uint8_t> input =
      makeErrorPayload(static_cast<uint8_t>(ErrorType::INVALID_FORMAT),
                       static_cast<uint16_t>(message.size()),
                       TestHelpers::bytesFromString(message));

  const ErrorPayload result = ProtocolParser::parseErrorPayload(input);

  EXPECT_EQ(result.code, ErrorType::INVALID_FORMAT);
  EXPECT_EQ(result.message, message);
}

TEST(ProtocolParserError, should_reject_payload_shorter_than_fixed_fields) {
  const std::vector<uint8_t> input = {0x00, 0x00};

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidSize);
}

TEST(ProtocolParserError, should_reject_unknown_error_code) {
  const std::vector<uint8_t> input =
      makeErrorPayload(TestHelpers::INVALID_ENUM_VALUE, 0, {});

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserError,
     should_reject_when_declared_message_length_mismatches_payload_size) {
  const std::vector<uint8_t> input =
      makeErrorPayload(static_cast<uint8_t>(ErrorType::EXECUTION_FAILED), 10,
                       TestHelpers::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseErrorPayload(input), InvalidSize);
}
