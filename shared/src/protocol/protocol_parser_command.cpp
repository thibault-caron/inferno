#include "protocol/protocol_parser.hpp"

namespace {

CommandType toCommandType(uint8_t value) {
  if (value >= static_cast<uint8_t>(CommandType::END)) {
    throw InvalidFieldValue("command_type",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<CommandType>(value);
}

}  // namespace

CommandPayload ProtocolParser::parseCommandPayload(
    const std::vector<uint8_t>& input) {
  if (input.size() < COMMAND_FIXED_BYTES) {
    throw InvalidSize("command payload", std::to_string(input.size()));
  }

  const CommandType type = toCommandType(input[2]);
  const uint16_t dataLen = ConvertEndian::readU16BE(input, 3);
  
  size_t expectedSize = COMMAND_FIXED_BYTES + dataLen;
  validateExpectedLength(input, expectedSize);

  CommandPayload payload;
  payload.id = ConvertEndian::readU16BE(input, 0);
  payload.type = type;

  if (payload.type == CommandType::SHELL && dataLen != 0) {
    payload.data.assign(
        reinterpret_cast<const char*>(input.data() + COMMAND_FIXED_BYTES),
        dataLen);
  }

  return payload;
}
