#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_register_payload_through_serialize_then_parse) {
  // Arrange
  RegisterPayload input{OSType::LINUX, ArchType::X64, "agent-01"};

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeRegisterPayload(input);
  const RegisterPayload result = ProtocolParser::parseRegisterPayload(bytes);

  // Assert
  EXPECT_EQ(result.os_type, input.os_type);
  EXPECT_EQ(result.arch, input.arch);
  EXPECT_EQ(result.hostname, input.hostname);
}

TEST(ProtocolRoundTrip,
     should_preserve_command_payload_through_serialize_then_parse) {
  // Arrange
  CommandPayload input{42, CommandType::SHELL, "whoami"};

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeCommandPayload(input);
  const CommandPayload result = ProtocolParser::parseCommandPayload(bytes);

  // Assert
  EXPECT_EQ(result.id, input.id);
  EXPECT_EQ(result.type, input.type);
  EXPECT_EQ(result.data, input.data);
}

TEST(ProtocolRoundTrip,
     should_preserve_response_payload_through_serialize_then_parse) {
  // Arrange
  ResponsePayload input{9, ResponseStatus::OK, 3, 1, "chunk-data"};

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeResponsePayload(input);
  const ResponsePayload result = ProtocolParser::parseResponsePayload(bytes);

  // Assert
  EXPECT_EQ(result.id, input.id);
  EXPECT_EQ(result.status, input.status);
  EXPECT_EQ(result.total_chunks, input.total_chunks);
  EXPECT_EQ(result.chunk_index, input.chunk_index);
  EXPECT_EQ(result.data, input.data);
}

TEST(ProtocolRoundTrip,
     should_preserve_data_payload_through_serialize_then_parse) {
  // Arrange
  DataPayload input{DataType::KEYLOGGER, "keys"};

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDataPayload(input);
  const DataPayload result = ProtocolParser::parseDataPayload(bytes);

  // Assert
  EXPECT_EQ(result.subtype, input.subtype);
  EXPECT_EQ(result.data, input.data);
}

TEST(ProtocolRoundTrip,
     should_preserve_error_payload_through_serialize_then_parse) {
  // Arrange
  ErrorPayload input{ErrorType::EXECUTION_FAILED, "boom"};

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeErrorPayload(input);
  const ErrorPayload result = ProtocolParser::parseErrorPayload(bytes);

  // Assert
  EXPECT_EQ(result.code, input.code);
  EXPECT_EQ(result.message, input.message);
}
