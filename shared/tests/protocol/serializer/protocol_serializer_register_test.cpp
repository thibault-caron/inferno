#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"
#include "protocol/protocol_test_helpers.hpp"

TEST(ProtocolSerializerRegister,
     should_produce_corresponding_byteArray_when_registerPayload_is_valid) {
  // Arrange
  const RegisterPayload input{OSType::LINUX, ArchType::X64, "agent-01"};
  const std::vector<std::uint8_t> expected = {
      static_cast<std::uint8_t>(OSType::LINUX),
      static_cast<std::uint8_t>(ArchType::X64),
      0x00,
      0x08,
      'a',
      'g',
      'e',
      'n',
      't',
      '-',
      '0',
      '1'};

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeRegisterPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidSize_when_hostname_is_empty) {
  // Arrange
  const RegisterPayload input{OSType::WINDOWS, ArchType::X86, ""};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidSize);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidFieldValue_when_os_type_is_unknown) {
  // Arrange
  const RegisterPayload input{
      static_cast<OSType>(TestHelpers::INVALID_ENUM_VALUE), ArchType::X64,
      "host"};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidFieldValue_when_arch_is_unknown) {
  // Arrange
  const RegisterPayload input{
      OSType::LINUX, static_cast<ArchType>(TestHelpers::INVALID_ENUM_VALUE),
      "host"};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerRegister,
     should_throw_InvalidSize_when_hostname_size_exceeds_max) {
  // Arrange
  const RegisterPayload input{OSType::LINUX, ArchType::X64,
                              std::string(REGISTER_MAX_HOSTNAME_LEN + 1, 'a')};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeRegisterPayload(input),
               InvalidSize);
}
