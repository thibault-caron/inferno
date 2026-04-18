#include "protocol/protocol_serializer.hpp"

#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"

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
     should_produce_corresponding_byteArray_when_errorPayload_is_valid) {
  // Arrange
  const ErrorPayload error = {
      ErrorType::EXECUTION_FAILED,
      "err",
  };

  // const std::vector<std::uint8_t> expected = {
  //     static_cast<std::uint8_t>(error.code),
  //     0b00000000,
  //     0b00000011,
  //     error.message[0],
  //     error.message[1],
  //     error.message[2]};
  std::vector<std::uint8_t> expected = {
      static_cast<std::uint8_t>(error.code),
      0b00000000,
      0b00000011,
  };
  // Append message bytes cleanly, no casting needed
  expected.insert(expected.end(), error.message.begin(), error.message.end());

  // Act
  const std::vector<std::uint8_t> result =
      ProtocolSerializer::serializeErrorPayload(error);

  // Assert
  EXPECT_EQ(expected, result);
}