#include "protocol/protocol_parser.hpp"

#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"

// TEST(SharedSanity, AlwaysPasses) { EXPECT_EQ(1 + 1, 2); }

// GroupName = usually the class you are testing
// TestName = what is specific behavior you are testing
TEST(ProtocolParser, should_produce_header_when_input_is_valid) {
  // Arrange
  std::vector<uint8_t> input = {
      'L',
      'P',
      'T',
      'F',                                         // identifier
      LPTF_VERSION,                                // version
      static_cast<uint8_t>(MessageType::COMMAND),  // type
      0x00,
      0x00  // size (0 in little-endian)
  };

  LptfHeader expected{
      {LPTF_IDENTIFIER[0], LPTF_IDENTIFIER[1], LPTF_IDENTIFIER[2],
       LPTF_IDENTIFIER[3]},
      LPTF_VERSION,
      MessageType::COMMAND,
      0,
  };

  // Act
  LptfHeader result = ProtocolParser::parseHeader(input);
  std::string_view resultId(result.identifier, 4);

  // Assert
  EXPECT_EQ(LPTF_IDENTIFIER_STR, resultId);
  EXPECT_EQ(expected.version, result.version);
  EXPECT_EQ(expected.type, result.type);
  EXPECT_EQ(expected.size, result.size);
}

TEST(ProtocolParser,
     should_trow_invalidIdentifier_when_wrong_identifier_provided) {
  // Arrange
  std::vector<uint8_t> input = {
      'L',
      'O',
      'T',
      'F',                                         // identifier
      LPTF_VERSION,                                // version
      static_cast<uint8_t>(MessageType::COMMAND),  // type
      0x00,
      0x00  // size (0 in little-endian)
  };

  // Act & Assert
  EXPECT_THROW(ProtocolParser::parseHeader(input), InvalidIdentifier);
}

TEST(ProtocolParser, should_trow_invalidSize_when_input_is_too_small) {
  // Arrange
  std::vector<uint8_t> input = {
      'L',
      'P',
      'T',
      'F',                                         // identifier
      LPTF_VERSION,                                // version
      static_cast<uint8_t>(MessageType::COMMAND),  // type
      0x00};

  // Act & Assert
  EXPECT_THROW(ProtocolParser::parseHeader(input), InvalidSize);
}

TEST(ProtocolParser, should_trow_invalidType_when_wrong_type_is_provided) {
  // Arrange
  std::vector<uint8_t> input = {
      'L',
      'P',
      'T',
      'F',                                           // identifier
      LPTF_VERSION,                                  // version
      static_cast<uint8_t>(MessageType::ERROR) + 1,  // type
      0x00,
      0x00  // size (0 in little-endian)
  };

  // Act & Assert
  EXPECT_THROW(ProtocolParser::parseHeader(input), InvalidType);
}

TEST(ProtocolParser, should_trow_UnsupportedVersion_when_wrong_version_type_is_provided) {
  // Arrange
  std::vector<uint8_t> input = {
      'L',
      'P',
      'T',
      'F',                                         // identifier
      'A',                                         // version
      static_cast<uint8_t>(MessageType::COMMAND),  // type
      0x00,
      0x00  // size (0 in little-endian)
  };

  // Act & Assert
  EXPECT_THROW(ProtocolParser::parseHeader(input), UnsupportedVersion);
}