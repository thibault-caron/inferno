#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/frame_builder.hpp"
#include "builders/os_info_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_os_info_payload_through_serialize_then_parse) {
  OsInfoPayload input = FrameBuilder::makeOsInfoPayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeOsInfoPayload(input);
  const OsInfoPayload result = ProtocolParser::parseOsInfoPayload(bytes);

  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_command_payload_through_serialize_then_parse) {
  CommandPayload input = FrameBuilder::makeCommandPayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeCommandPayload(input);
  const CommandPayload result = ProtocolParser::parseCommandPayload(bytes);

  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_response_payload_through_serialize_then_parse) {
  ResponsePayload input = FrameBuilder::makeResponsePayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeResponsePayload(input);
  const ResponsePayload result = ProtocolParser::parseResponsePayload(bytes);
  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_data_payload_through_serialize_then_parse) {
  DataPayload input{DataType::METRICS_SAMPLE, {'k', 'e', 'y', 's'}};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDataPayload(input);
  const DataPayload result = ProtocolParser::parseDataPayload(bytes);

  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip,
     should_preserve_error_payload_through_serialize_then_parse) {
  ErrorPayload input{ErrorType::EXECUTION_FAILED, "boom"};

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeErrorPayload(input);
  const ErrorPayload result = ProtocolParser::parseErrorPayload(bytes);
  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundTrip, should_preserve_header_through_serialize_then_parse) {
  LptfHeader input;
  input.identifier = LPTF_IDENTIFIER;
  input.type = MessageType::COMMAND;

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeHeader(input);
  const LptfHeader result = ProtocolParser::parseHeader(bytes);
  EXPECT_EQ(result, input);
}

TEST(ProtocolRoundRrip,
     should_preserve_dashboard_command_through_serialize_then_parse) {
  DashboardCommand command;
  command.target = "targetId";
  command.sent_at = "2026-07-28T14:32:45Z";
  command.command = FrameBuilder::makeCommandPayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDashboardCommand(command);
  const DashboardCommand result = ProtocolParser::parseDashboardCommand(bytes);
  EXPECT_EQ(result, command);
}

TEST(ProtocolRoundRrip,
     should_preserve_dashboard_data_through_serialize_then_parse) {
  DashboardData data;
  data.target = "targetId";
  data.data = FrameBuilder::makeDataPayload();
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDashboardData(data);
  const DashboardData result = ProtocolParser::parseDashboardData(bytes);
  EXPECT_EQ(result, data);
}

TEST(ProtocolRoundRrip,
     should_preserve_dashboard_disconnect_through_serialize_then_parse) {
  DashboardDisconnect disconnect;
  disconnect.target = "targetId";

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDashboardDisconnect(disconnect);
  const DashboardDisconnect result =
      ProtocolParser::parseDashboardDisconnect(bytes);
  EXPECT_EQ(result, disconnect);
}

TEST(ProtocolRoundRrip,
     should_preserve_dashboard_response_through_serialize_then_parse) {
  DashboardResponse response;
  response.target = "targetId";
  response.received_at = "2026-07-28T14:32:45Z";
  response.response = FrameBuilder::makeResponsePayload();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeDashboardResponse(response);
  const DashboardResponse result =
      ProtocolParser::parseDashboardResponse(bytes);
  EXPECT_EQ(result, response);
}
