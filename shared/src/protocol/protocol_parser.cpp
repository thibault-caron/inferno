#include "protocol/protocol_parser.hpp"


LptfHeader ProtocolParser::parseHeader(const std::vector<uint8_t>& input) {
  if (input.size() < 8) {
    throw InvalidSize(std::string("header"), std::to_string(input.size()));
  }

  std::string_view inputIdentifier(reinterpret_cast<const char*>(input.data()),
                                   4);
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(std::string(inputIdentifier));
  }
  // Validate identifier
  //   if (input[0] != LPTF_IDENTIFIER[0] || input[1] != LPTF_IDENTIFIER[1] ||
  //       input[2] != LPTF_IDENTIFIER[2] || input[3] != LPTF_IDENTIFIER[3]) {
  //     std::string wrongIdentifier(reinterpret_cast<const
  //     char*>(input.data()), 4); throw InvalidIdentifier(wrongIdentifier);
  //   }

  LptfHeader header;
  for (std::size_t i = 0; i < 4; ++i) {
    header.identifier[i] = static_cast<char>(input[i]);
  }

  header.version = input[4];
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version), "Version provided is not a number");
  }
  header.type = toMessageType(input[5]);
  header.size = ConvertEndian::readU16BE(input, 6);
  return header;
}

MessageType ProtocolParser::toMessageType(uint8_t value) {
  if (value > static_cast<uint8_t>(MessageType::ERROR)) {
    throw InvalidType(std::to_string(static_cast<uint8_t>(value)));
  }
  return static_cast<MessageType>(value);
}
