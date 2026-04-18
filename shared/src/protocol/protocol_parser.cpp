#include "protocol/protocol_parser.hpp"

LptfHeader ProtocolParser::parseHeader(const std::vector<std::uint8_t>& input) {
  if (input.size() < 8) {
    throw InvalidSize(std::string("header"), std::to_string(input.size()));
  }

  const std::string_view inputIdentifier(
      reinterpret_cast<const char*>(input.data()), 4);
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(std::string(inputIdentifier));
  }

  LptfHeader header;
  for (std::size_t i = 0; i < 4; ++i) {
    header.identifier[i] = static_cast<char>(input[i]);
  }

  header.version = input[4];
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  header.type = toMessageType(input[5]);
  header.size = ConvertEndian::readU16BE(input, 6);
  return header;
}

MessageType ProtocolParser::toMessageType(std::uint8_t value) {
  if (value > static_cast<std::uint8_t>(MessageType::ERROR)) {
    throw InvalidType(std::to_string(static_cast<std::uint8_t>(value)));
  }
  return static_cast<MessageType>(value);
}

// TODO  does copying std::uint16_t cost less than using a reference ? Const
// anyway ?
void ProtocolParser::validateStringLength(
    const std::uint16_t length, const std::vector<std::uint8_t>& input,
    const std::size_t MAX_LEN, const std::size_t expectedSize) {
  validateNotNullLength(length, MAX_LEN);
  validateExpectedLength(input, expectedSize);
}

void ProtocolParser::validateNotNullLength(const std::uint16_t length,
                                           const std::size_t MAX_LEN) {
  if (length == 0 || length > MAX_LEN) {
    throw InvalidSize("Struct string length", std::to_string(length));
  }
}

void ProtocolParser::validateExpectedLength(const std::vector<std::uint8_t>& input,
                              const std::size_t expectedSize) {
  if (input.size() != expectedSize) {
    throw InvalidSize("Payload", std::to_string(input.size()));
  }
}