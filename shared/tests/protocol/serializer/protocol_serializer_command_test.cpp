#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"
#include "protocol/protocol_test_helpers.hpp"

TEST(ProtocolSerializerCommand,
     should_produce_corresponding_byteArray_when_shell_command_is_valid) {
  // Arrange
  const CommandPayload input{42, CommandType::SHELL, "whoami"};
  const std::vector<std::uint8_t> expected = {
      0x00, 0x2A, static_cast<std::uint8_t>(CommandType::SHELL),
      0x00, 0x06, 'w',
      'h',  'o',  'a',
      'm',  'i'};

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeCommandPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerCommand,
     should_produce_corresponding_byteArray_when_os_info_command_is_valid) {
  // Arrange
  const CommandPayload input{7, CommandType::OS_INFO, ""};
  const std::vector<std::uint8_t> expected = {
      0x00, 0x07, static_cast<std::uint8_t>(CommandType::OS_INFO), 0x00, 0x00};

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeCommandPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerCommand,
     should_throw_InvalidSize_when_non_shell_command_contains_data) {
  // Arrange
  const CommandPayload input{7, CommandType::OS_INFO, "abc"};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeCommandPayload(input), InvalidSize);
}

TEST(ProtocolSerializerCommand,
     should_throw_InvalidFieldValue_when_shell_command_contains_no_data) {
  // Arrange
  const CommandPayload input{7, CommandType::SHELL, ""};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeCommandPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerCommand,
     should_throw_InvalidFieldValue_when_command_type_is_unknown) {
  // Arrange
  const CommandPayload input{
      7, static_cast<CommandType>(TestHelpers::INVALID_ENUM_VALUE), ""};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeCommandPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerCommand,
     should_throw_InvalidSize_when_command_data_is_too_large) {
  // Arrange
  const CommandPayload input{
      42, CommandType::SHELL,
      std::string(static_cast<std::size_t>(65535) + 1, 'a')};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeCommandPayload(input), InvalidSize);
}
