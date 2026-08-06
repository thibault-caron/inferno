#include "codec/protocol_serializer.hpp"

#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "exception/lptf_exception.hpp"
#include "fixtures/common.hpp"
#include "builders/frame_builder.hpp"

TEST(ProtocolSerializer,
     should_produce_corresponding_byteArray_when_input_is_valid) {
  // Arrange
  const LptfHeader input = {
      {LPTF_IDENTIFIER[0], LPTF_IDENTIFIER[1], LPTF_IDENTIFIER[2],
       LPTF_IDENTIFIER[3]},
      LPTF_VERSION,
      MessageType::COMMAND,
      300,  // 00000001 00101100 big endian
  };

  const std::vector<std::uint8_t> expected = {
      'L',
      'P',
      'T',
      'F',                                              // identifier
      LPTF_VERSION,                                     // version
      static_cast<std::uint8_t>(MessageType::COMMAND),  // type
      0b00000001,                                       // high byte of 300
      0b00101100                                        // low byte of 300
  };

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeHeader(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializer,
         should_throw_InvalidIdentifier_when_header_identifier_is_invalid) {
    // Arrange
    const LptfHeader input = {
            {'B', 'A', 'D', '!'},
            LPTF_VERSION,
            MessageType::COMMAND,
            1,
    };

    // Act & Assert
    EXPECT_THROW(ProtocolSerializer::serializeHeader(input), InvalidIdentifier);
}

TEST(ProtocolSerializer,
         should_throw_UnsupportedVersion_when_header_version_is_invalid) {
    // Arrange
    const LptfHeader input = {
            {LPTF_IDENTIFIER[0], LPTF_IDENTIFIER[1], LPTF_IDENTIFIER[2],
             LPTF_IDENTIFIER[3]},
            static_cast<std::uint8_t>(LPTF_VERSION + 1),
            MessageType::COMMAND,
            1,
    };

    // Act & Assert
    EXPECT_THROW(ProtocolSerializer::serializeHeader(input), UnsupportedVersion);
}

TEST(ProtocolSerializer,
         should_throw_InvalidType_when_header_message_type_is_unknown) {
    // Arrange
    const LptfHeader input = {
            {LPTF_IDENTIFIER[0], LPTF_IDENTIFIER[1], LPTF_IDENTIFIER[2],
             LPTF_IDENTIFIER[3]},
            LPTF_VERSION,
            static_cast<MessageType>(255),
            1,
    };

    // Act & Assert
    EXPECT_THROW(ProtocolSerializer::serializeHeader(input), InvalidType);
}