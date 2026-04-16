#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"
#include "protocol/protocol_test_helpers.hpp"

TEST(ProtocolSerializerData,
     should_produce_corresponding_byteArray_when_data_payload_is_valid) {
  // Arrange
  DataPayload input{DataType::KEYLOGGER, "keys"};
  std::vector<uint8_t> expected = {static_cast<uint8_t>(DataType::KEYLOGGER),
                                   0x00,
                                   0x04,
                                   'k',
                                   'e',
                                   'y',
                                   's'};

  // Act
  std::vector<uint8_t> result = ProtocolSerializer::serializeDataPayload(input);

  // Assert
  EXPECT_EQ(expected, result);
}

TEST(ProtocolSerializerData,
     should_throw_InvalidFieldValue_when_data_subtype_is_unknown) {
  // Arrange
  DataPayload input{static_cast<DataType>(TestHelpers::INVALID_ENUM_VALUE), ""};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeDataPayload(input),
               InvalidFieldValue);
}

TEST(ProtocolSerializerData, should_throw_InvalidSize_when_data_is_too_large) {
  // Arrange
  DataPayload input{DataType::KEYLOGGER,
                    std::string(static_cast<std::size_t>(65535) + 1, 'a')};

  // Act & Assert
  EXPECT_THROW(ProtocolSerializer::serializeDataPayload(input), InvalidSize);
}
