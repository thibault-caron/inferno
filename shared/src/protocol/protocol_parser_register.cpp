#include "protocol/protocol_parser.hpp"

namespace {
// TODO const for argument almost everywhere ?
OSType toOsType(std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(OSType::END)) {
    throw InvalidFieldValue("os_type", std::to_string(value));
  }
  return static_cast<OSType>(value);
}

ArchType toArchType(std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ArchType::END)) {
    throw InvalidFieldValue("arch", std::to_string(value));
  }
  return static_cast<ArchType>(value);
}

}  // namespace

RegisterPayload ProtocolParser::parseRegisterPayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < REGISTER_FIXED_BYTES) {
    throw InvalidSize("register payload", std::to_string(input.size()));
  }
  const std::uint16_t hostnameLen{ConvertEndian::readU16BE(input, 2)};

  const std::size_t maxHostnameLength{MAX_VALUE_INT16 - REGISTER_FIXED_BYTES};
  const std::size_t expectedSize{REGISTER_FIXED_BYTES + hostnameLen};
  validateStringLength(hostnameLen, input, maxHostnameLength, expectedSize);

  RegisterPayload payload;
  payload.os_type = toOsType(input[0]);
  payload.arch = toArchType(input[1]);
  payload.hostname.assign(
      reinterpret_cast<const char*>(input.data() + REGISTER_FIXED_BYTES),
      hostnameLen);
  return payload;
}