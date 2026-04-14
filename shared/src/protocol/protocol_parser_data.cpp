#include "protocol/protocol_parser.hpp"

namespace {

void ensureDataMinSize(const std::vector<uint8_t>& input) {
  if (input.size() < DATA_FIXED_BYTES) {
    throw InvalidSize("data payload", std::to_string(input.size()));
  }
}

DataType toDataType(uint8_t value) {
  if (value <= static_cast<uint8_t>(DataType::KEYLOGGER)) {
    return static_cast<DataType>(value);
  }
  throw InvalidFieldValue("data_type",
                          std::to_string(static_cast<unsigned int>(value)));
}

uint16_t readDataLen(const std::vector<uint8_t>& input) {
  return ConvertEndian::readU16BE(input, 1);
}

void validateDataSize(uint16_t dataLen, const std::vector<uint8_t>& input) {
  const std::size_t expectedSize = DATA_FIXED_BYTES + dataLen;
  if (input.size() != expectedSize) {
    throw InvalidSize("data payload", std::to_string(input.size()));
  }
}

}  // namespace

DataPayload ProtocolParser::parseDataPayload(
    const std::vector<uint8_t>& input) {
  ensureDataMinSize(input);
  const uint16_t dataLen = readDataLen(input);
  validateDataSize(dataLen, input);

  DataPayload payload;
  payload.subtype = toDataType(input[0]);
  payload.data.assign(
      reinterpret_cast<const char*>(input.data() + DATA_FIXED_BYTES), dataLen);
  return payload;
}
