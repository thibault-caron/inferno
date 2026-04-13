#include "protocol/protocol_parser.hpp"

namespace {

void ensureRegisterMinSize(const std::vector<uint8_t>& input) {
  if (input.size() < REGISTER_FIXED_BYTES) {
    throw InvalidSize("register payload", std::to_string(input.size()));
  }
}

uint16_t readHostnameLen(const std::vector<uint8_t>& input) {
  return ConvertEndian::readU16BE(input, 2);
}

OSType toOsType(uint8_t value) {
  if (value <= static_cast<uint8_t>(OSType::MAC)) {
    return static_cast<OSType>(value);
  }
  throw InvalidFieldValue("os_type", std::to_string(value));
}

ArchType toArchType(uint8_t value) {
  if (value <= static_cast<uint8_t>(ArchType::ARM)) {
    return static_cast<ArchType>(value);
  }
  throw InvalidFieldValue("arch", std::to_string(value));
}

void validateRegisterSize(uint16_t hostnameLen,
                          const std::vector<uint8_t>& input) {
  if (hostnameLen == 0 || hostnameLen > REGISTER_MAX_HOSTNAME_LEN) {
    throw InvalidSize("register hostname length", std::to_string(hostnameLen));
  }
  const std::size_t expectedSize = REGISTER_FIXED_BYTES + hostnameLen;
  if (input.size() != expectedSize) {
    throw InvalidSize("register payload", std::to_string(input.size()));
  }
}

}  // namespace

RegisterPayload ProtocolParser::parseRegisterPayload(
    const std::vector<uint8_t>& input) {
  ensureRegisterMinSize(input);
  const uint16_t hostnameLen = readHostnameLen(input);
  validateRegisterSize(hostnameLen, input);

  RegisterPayload payload;
  payload.os_type = toOsType(input[0]);
  payload.arch = toArchType(input[1]);
  payload.hostname.assign(
      reinterpret_cast<const char*>(input.data() + REGISTER_FIXED_BYTES),
      hostnameLen);
  return payload;
}