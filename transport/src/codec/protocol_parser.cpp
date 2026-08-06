#include "codec/protocol_parser.hpp"

#include <cstring>
#include <string>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "exception/lptf_exception.hpp"
#include "logger.hpp"

namespace ProtocolParser {
LptfHeader parseHeader(const std::vector<std::uint8_t>& input) {
  if (input.size() < 8) {
    throw InvalidSize(std::string("header"), std::to_string(input.size()));
  }

  const std::string_view inputIdentifier(
      reinterpret_cast<const char*>(input.data()), 4);
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(std::string(inputIdentifier));
  }

  LptfHeader header;
  std::size_t offset{4};
  // for (std::size_t i = 0; i < offset; ++i) {
  //   header.identifier[i] = static_cast<char>(input[i]);
  // }
  std::copy(input.begin(), input.begin() + 4, header.identifier.begin());

  header.version = input[offset];
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  offset++;
  header.type = ProtocolHelper::EnumConversion::toMessageType(input[offset]);
  offset++;
  header.size = ConvertEndian::readU16BE(input, offset);
  return header;
}

DataPayload parseDataPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < DATA_FIXED_BYTES) {
    throw InvalidSize("data payload", std::to_string(input.size()));
  }
  std::size_t offset{1};
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, offset)};
  const std::size_t expectedSize{DATA_FIXED_BYTES + dataLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);
  DataPayload payload;
  payload.subtype = ProtocolHelper::EnumConversion::toDataType(input[0]);
  payload.data =
      std::vector<std::uint8_t>(input.begin() + DATA_FIXED_BYTES,
                                input.begin() + DATA_FIXED_BYTES + dataLen);
  return payload;
}

CommandPayload parseCommandPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < COMMAND_FIXED_BYTES) {
    throw InvalidSize("command payload", std::to_string(input.size()));
  }
  // std::size_t typeOffset{4};
  std::size_t typeOffset{0};
  CommandPayload payload;
  payload.id = ConvertEndian::readU32BE(input, typeOffset);

  const CommandType type =
      ProtocolHelper::EnumConversion::toCommandType(input[typeOffset]);
  payload.type = type;
  typeOffset++;

  const std::uint16_t dataLen = ConvertEndian::readU16BE(input, typeOffset);
  const std::size_t expectedSize{COMMAND_FIXED_BYTES + dataLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);

  // std::size_t payloadOffset{0};

  if (payload.type == CommandType::SHELL && dataLen != 0) {
    payload.data.assign(
        reinterpret_cast<const char*>(input.data() + COMMAND_FIXED_BYTES),
        dataLen);
  }

  return payload;
}

ResponsePayload parseResponsePayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < RESPONSE_FIXED_BYTES) {
    throw InvalidSize("response payload", std::to_string(input.size()));
  }
  // std::size_t offset{3};
  std::size_t offset{0};
  ResponsePayload payload;
  // std::size_t payloadOffset{0};
  payload.id = ConvertEndian::readU32BE(input, offset);

  payload.status =
      ProtocolHelper::EnumConversion::toResponseStatus(input[offset]);
  offset++;
  payload.type =
      ProtocolHelper::EnumConversion::toCommandType(input[offset]);
  offset++;
  payload.total_chunks = input[offset];
  offset++;
  payload.chunk_index = input[offset];
  offset++;
  ProtocolHelper::validateChunkFields(payload.total_chunks,
                                      payload.chunk_index);  // 4 & 5

  // offset += 2;
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, offset)};  // 5

  const std::size_t expectedSize{RESPONSE_FIXED_BYTES + dataLen};
  ProtocolHelper::validateExpectedLength(input, expectedSize);
  // payload.data.assign(
  //     reinterpret_cast<const char*>(input.data() + RESPONSE_FIXED_BYTES),
  //     dataLen);

  payload.data.assign(input.begin() + RESPONSE_FIXED_BYTES,
                      input.begin() + RESPONSE_FIXED_BYTES + dataLen);
  return payload;
}

ErrorPayload parseErrorPayload(const std::vector<std::uint8_t>& input) {
  if (input.size() < ERROR_FIXED_BYTES) {
    throw InvalidSize("error payload", std::to_string(input.size()));
  }

  std::size_t offset{1};
  const std::uint16_t messageLen{ConvertEndian::readU16BE(input, offset)};
  const std::size_t expectedSize{ERROR_FIXED_BYTES + messageLen};
  const std::size_t maxLength{MAX_VALUE_INT16 - ERROR_FIXED_BYTES};

  // if (input.size() != expectedSize) {
  //   throw InvalidSize("error payload", std::to_string(input.size()));
  // }

  ProtocolHelper::validateStringLength(messageLen, input, maxLength,
                                       expectedSize);

  ErrorPayload payload;
  payload.code = ProtocolHelper::EnumConversion::toErrorType(input[0]);
  payload.message.assign(
      reinterpret_cast<const char*>(input.data() + ERROR_FIXED_BYTES),
      messageLen);
  return payload;
}

DashboardCommand parseDashboardCommand(const std::vector<std::uint8_t>& input) {
  //       if (input.size() < PROCESS_INFO_FIXED_SIZE) {
  //   throw InvalidSize("process info payload", std::to_string(input.size()));
  // }

  DashboardCommand command;
  size_t offset = 0;

  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);
  std::uint16_t sentAtLen = ConvertEndian::readU16BE(input, offset);

  if (offset + targetLen + sentAtLen > input.size()) {
    throw InvalidSize("target name", std::to_string(targetLen));
  }

  command.target = ConvertEndian::getString(input, offset, targetLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }

  command.sent_at = ConvertEndian::getString(input, offset, sentAtLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }

  command.command = parseCommandPayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return command;
}

DashboardData parseDashboardData(const std::vector<std::uint8_t>& input) {
  DashboardData data;
  size_t offset = 0;

  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);

  if (offset + targetLen > input.size()) {
    throw InvalidSize("target name", std::to_string(targetLen));
  }

  data.target = ConvertEndian::getString(input, offset, targetLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }
  data.data = parseDataPayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return data;
}

DashboardResponse parseDashboardResponse(
    const std::vector<std::uint8_t>& input) {
  DashboardResponse response;
  size_t offset = 0;

  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);
  std::uint16_t receivedAtlen = ConvertEndian::readU16BE(input, offset);

  if (offset + targetLen + receivedAtlen > input.size()) {
    throw InvalidSize("target name", std::to_string(targetLen));
  }

  response.target = ConvertEndian::getString(input, offset, targetLen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }

  response.received_at = ConvertEndian::getString(input, offset, receivedAtlen);
  if (offset >= input.size()) {
    throw InvalidSize("dashboard command payload", "0");
  }

  response.response = parseResponsePayload(
      std::vector<std::uint8_t>(input.begin() + offset, input.end()));
  return response;
}

DashboardDisconnect parseDashboardDisconnect(
    const std::vector<std::uint8_t>& input) {
  DashboardDisconnect disconnect;
  size_t offset = 0;
  std::uint16_t targetLen = ConvertEndian::readU16BE(input, offset);

  ProtocolHelper::validateNotNullLength(targetLen, 17);

  if (offset + targetLen > input.size()) {
    throw InvalidSize("target", std::to_string(targetLen));
  }

  disconnect.target.assign(reinterpret_cast<const char*>(input.data() + offset),
                           targetLen);

  return disconnect;
}
}  // namespace ProtocolParser
