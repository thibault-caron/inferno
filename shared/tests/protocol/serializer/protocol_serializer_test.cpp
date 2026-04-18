#include <gtest/gtest.h>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolSerializer,
     should_produce_corresponding_byteArray_when_input_is_valid) {
  // Arrange
  LptfHeader input{
      {LPTF_IDENTIFIER[0], LPTF_IDENTIFIER[1], LPTF_IDENTIFIER[2],
       LPTF_IDENTIFIER[3]},
      LPTF_VERSION,
      MessageType::COMMAND,
      300,  // 00000001 00101100 big endian
  };

  std::vector<uint8_t> expected = {
      'L',
      'P',
      'T',
      'F',                                         // identifier
      LPTF_VERSION,                                // version
      static_cast<uint8_t>(MessageType::COMMAND),  // type
      0b00000001,                                  // high byte of 300
      0b00101100                                   // low byte of 300
  };

  // Act
  std::vector<uint8_t> result = ProtocolSerializer::serializeHeader(input);

  // Assert
  EXPECT_EQ(expected, result);
}


TEST(ProtocolSerializer,
     should_produce_corresponding_byteArray_when_errorPayload_is_valid) {
  // Arrange
  ErrorPayload error{
      ErrorType::EXECUTION_FAILED,
      "err", 
  };

  std::vector<uint8_t> expected = {
    static_cast<uint8_t>(error.code),
    0b00000000,
    0b00000011,
    error.message[0],
    error.message[1],
    error.message[2]
  };

  // Act
  std::vector<uint8_t> result = ProtocolSerializer::serializeErrorPayload(error);

  // Assert
  EXPECT_EQ(expected, result);
}