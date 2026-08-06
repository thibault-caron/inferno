#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/frame_builder.hpp"
#include "builders/os_info_builder.hpp"
#include "builders/process_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"

TEST(ProtocolRoundTrip,
     should_preserve_process_info_through_serialize_then_parse) {
  // Arrange
  ProcessInfo info = ProcessBuilder::createProcessInfo();

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeProcessInfo(info);
  const ProcessInfo result = ProtocolParser::parseProcessInfo(bytes);

  // Assert
  EXPECT_EQ(result, info);
}

TEST(ProtocolRoundTrip,
     should_preserve_process_info_list_through_serialize_then_parse) {
  // Arrange
  std::vector<ProcessInfo> infos = ProcessBuilder::createProcessInfoList();

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeProcessInfoList(infos);
  const std::vector<ProcessInfo> result =
      ProtocolParser::parseProcessInfoList(bytes);

  // Assert
  EXPECT_EQ(result, infos);
}

TEST(ProtocolRoundRrip,
     should_preserve_register_payload_through_serialize_then_parse) {
  RegisterPayload payload;
  payload.id = "whateverId";
  payload.registered_at = "2026-07-25T14:32:45Z";
  payload.last_seen = "2026-07-28T14:32:45Z";
  payload.online = true;
  payload.system = OsInfoBuilder::create();

  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeRegisterPayload(payload);
  const RegisterPayload result = ProtocolParser::parseRegisterPayload(bytes);
  EXPECT_EQ(result, payload);
}

TEST(ProtocolRoundTrip,
     should_preserve_register_payload_list_through_serialize_then_parse) {
  // Arrange
  std::vector<RegisterPayload> registerPayloads;
  for (std::size_t i = 0; i < 5; i++) {
    RegisterPayload payload;
    payload.id = "whateverId";
    payload.registered_at = "2026-07-25T14:32:45Z";
    payload.last_seen = "2026-07-28T14:32:45Z";
    payload.online = true;
    payload.system = OsInfoBuilder::create();
    registerPayloads.push_back(payload);
  }
  //   ProcessBuilder::createProcessInfoList();

  // Act
  const std::vector<std::uint8_t> bytes =
      ProtocolSerializer::serializeRegisterPayloadList(registerPayloads);
  const std::vector<RegisterPayload> result =
      ProtocolParser::parseRegisterPayloadList(bytes);

  // Assert
  EXPECT_EQ(result, registerPayloads);
}
