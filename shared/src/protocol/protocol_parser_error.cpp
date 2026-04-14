#include "protocol/protocol_parser.hpp"

namespace {

void ensureErrorMinSize(const std::vector<uint8_t>& input) {
  if (input.size() < ERROR_FIXED_BYTES) {
    throw InvalidSize("error payload", std::to_string(input.size()));
  }
}

ErrorType toErrorType(uint8_t value) {
  if (value <= static_cast<uint8_t>(ErrorType::SIZE_EXCEEDED)) {
    return static_cast<ErrorType>(value);
  }
  throw InvalidFieldValue("error_code",
                          std::to_string(static_cast<unsigned int>(value)));
}

uint16_t readMessageLen(const std::vector<uint8_t>& input) {
  return ConvertEndian::readU16BE(input, 1);
}

void validateErrorSize(uint16_t messageLen,
                       const std::vector<uint8_t>& input) {
  const std::size_t expectedSize = ERROR_FIXED_BYTES + messageLen;
  if (input.size() != expectedSize) {
    throw InvalidSize("error payload", std::to_string(input.size()));
  }
}

}  // namespace

ErrorPayload ProtocolParser::parseErrorPayload(
    const std::vector<uint8_t>& input) {
  ensureErrorMinSize(input);
  const uint16_t messageLen = readMessageLen(input);
  validateErrorSize(messageLen, input);

  ErrorPayload payload;
  payload.code = toErrorType(input[0]);
  payload.message.assign(
      reinterpret_cast<const char*>(input.data() + ERROR_FIXED_BYTES),
      messageLen);
  return payload;
}
