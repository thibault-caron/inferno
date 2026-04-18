#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_test_helpers.hpp"

namespace {

std::vector<std::uint8_t> makeRegisterPayload(
    const std::uint8_t rawOs, const std::uint8_t rawArch,
    const std::uint16_t declaredHostnameLen,
    const std::vector<std::uint8_t>& hostnameBytes) {
  std::vector<std::uint8_t> out;
  out.reserve(REGISTER_FIXED_BYTES + hostnameBytes.size());
  out.push_back(rawOs);
  out.push_back(rawArch);
  out.push_back(static_cast<std::uint8_t>((declaredHostnameLen >> 8) & 0xFF));
  out.push_back(static_cast<std::uint8_t>(declaredHostnameLen & 0xFF));
  out.insert(out.end(), hostnameBytes.begin(), hostnameBytes.end());
  return out;
}

std::vector<std::uint8_t> bytesFromString(const std::string& string) {
  return std::vector<std::uint8_t>(string.begin(), string.end());
}

}  // namespace

TEST(ProtocolParserRegister,
     should_parse_register_payload_when_input_is_valid) {
  const std::string hostname = "client-01";
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      static_cast<std::uint16_t>(hostname.size()), bytesFromString(hostname));
  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.os_type, OSType::LINUX);
  EXPECT_EQ(result.arch, ArchType::X64);
  EXPECT_EQ(result.hostname, hostname);
}

TEST(ProtocolParserRegister, should_reject_empty_hostname) {
  const std::vector<std::uint8_t> input =
      makeRegisterPayload(static_cast<std::uint8_t>(OSType::WINDOWS),
                          static_cast<std::uint8_t>(ArchType::X86), 0, {});
  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_accept_max_hostname_length) {
  const std::string hostname(REGISTER_MAX_HOSTNAME_LEN, 'a');
  const std::vector<std::uint8_t> input =
      makeRegisterPayload(static_cast<std::uint8_t>(OSType::MAC),
                          static_cast<std::uint8_t>(ArchType::ARM),
                          REGISTER_MAX_HOSTNAME_LEN, bytesFromString(hostname));
  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.os_type, OSType::MAC);
  EXPECT_EQ(result.arch, ArchType::ARM);
  EXPECT_EQ(result.hostname.size(), hostname.size());
  EXPECT_EQ(result.hostname.front(), 'a');
  EXPECT_EQ(result.hostname.back(), 'a');
}

TEST(ProtocolParserRegister,
     should_reject_payload_shorter_than_fixed_register_fields) {
  const std::vector<std::uint8_t> input = {
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64), 0x00};

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister,
     should_reject_when_declared_hostname_length_exceeds_available_bytes) {
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64), 10, bytesFromString("abc"));
  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidSize);
}

TEST(ProtocolParserRegister, should_decode_hostname_length_as_big_endian) {
  const std::string hostname(300, 'z');
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::WINDOWS),
      static_cast<std::uint8_t>(ArchType::X86), 300, bytesFromString(hostname));
  const RegisterPayload result = ProtocolParser::parseRegisterPayload(input);

  EXPECT_EQ(result.hostname.size(), 300u);
  EXPECT_EQ(result.hostname[0], 'z');
  EXPECT_EQ(result.hostname[299], 'z');
}

TEST(ProtocolParserRegister, should_reject_unknown_os_type) {
  const std::string hostname = "host-a";
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      TestHelpers::INVALID_ENUM_VALUE, static_cast<std::uint8_t>(ArchType::X64),
      static_cast<std::uint16_t>(hostname.size()), bytesFromString(hostname));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidFieldValue);
}

TEST(ProtocolParserRegister, should_reject_unknown_arch) {
  const std::string hostname = "host-b";
  const std::vector<std::uint8_t> input = makeRegisterPayload(
      static_cast<std::uint8_t>(OSType::LINUX), TestHelpers::INVALID_ENUM_VALUE,
      static_cast<std::uint16_t>(hostname.size()), bytesFromString(hostname));

  EXPECT_THROW(ProtocolParser::parseRegisterPayload(input), InvalidFieldValue);
}