#include "protocol/protocol_parser.hpp"

namespace {

ErrorType toErrorType(uint8_t value) {
  if (value >= static_cast<uint8_t>(ErrorType::END)) {
    throw InvalidFieldValue("error_code",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<ErrorType>(value);
}

}  // namespace

ErrorPayload ProtocolParser::parseErrorPayload(
    const std::vector<uint8_t>& input) {
  if (input.size() < ERROR_FIXED_BYTES) {
    throw InvalidSize("error payload", std::to_string(input.size()));
  }

  const uint16_t messageLen = ConvertEndian::readU16BE(input, 1);
  const std::size_t expectedSize = ERROR_FIXED_BYTES + messageLen;
  const std::size_t maxLength = MAX_VALUE_INT16 - ERROR_FIXED_BYTES;

  // if (input.size() != expectedSize) {
  //   throw InvalidSize("error payload", std::to_string(input.size()));
  // }

  validateStringLength(messageLen, input, maxLength, expectedSize);

  ErrorPayload payload;
  payload.code = toErrorType(input[0]);
  payload.message.assign(
      reinterpret_cast<const char*>(input.data() + ERROR_FIXED_BYTES),
      messageLen);
  return payload;
}
