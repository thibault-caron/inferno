#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_test_helpers.hpp"

namespace {

std::vector<uint8_t> makeDataPayload(uint8_t rawSubtype, uint16_t declaredLen,
                                     const std::vector<uint8_t>& dataBytes) {
  std::vector<uint8_t> out;
  out.resize(DATA_FIXED_BYTES + dataBytes.size());
  out[0] = rawSubtype;
  ConvertEndian::writeU16BE(out, 1, declaredLen);
  for (std::size_t i = 0; i < dataBytes.size(); ++i) {
    out[DATA_FIXED_BYTES + i] = dataBytes[i];
  }
  return out;
}

}  // namespace

TEST(ProtocolParserData, should_parse_data_payload_when_input_is_valid) {
  const std::string data = "keys";
  const std::vector<uint8_t> input = makeDataPayload(
      static_cast<uint8_t>(DataType::KEYLOGGER),
      static_cast<uint16_t>(data.size()), TestHelpers::bytesFromString(data));

  const DataPayload result = ProtocolParser::parseDataPayload(input);

  EXPECT_EQ(result.subtype, DataType::KEYLOGGER);
  EXPECT_EQ(result.data, data);
}

TEST(ProtocolParserData, should_reject_payload_shorter_than_fixed_fields) {
  const std::vector<uint8_t> input = {0x00, 0x00};

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidSize);
}

TEST(ProtocolParserData, should_reject_unknown_data_subtype) {
  const std::vector<uint8_t> input =
      makeDataPayload(TestHelpers::INVALID_ENUM_VALUE, 0, {});

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserData,
     should_reject_when_declared_data_length_mismatches_payload_size) {
  const std::vector<uint8_t> input =
      makeDataPayload(static_cast<uint8_t>(DataType::KEYLOGGER), 10,
                      TestHelpers::bytesFromString("abc"));

  EXPECT_THROW(ProtocolParser::parseDataPayload(input), InvalidSize);
}
