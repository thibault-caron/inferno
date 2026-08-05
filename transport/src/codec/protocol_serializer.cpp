#include "codec/protocol_serializer.hpp"

#include <cstddef>
#include <string>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "exception/lptf_exception.hpp"
#include "logger.hpp"

namespace ProtocolSerializer {
std::vector<std::uint8_t> serializeHeader(const LptfHeader& header) {
  ProtocolHelper::validateHeader(header);

  std::vector<std::uint8_t> headerInByte(LPTF_HEADER_SIZE);

  // for (std::size_t i = 0; i < sizeof(header.identifier); ++i) {
  //   headerInByte[i] = header.identifier[i];
  // }
  std::copy(header.identifier.begin(), header.identifier.end(),
            headerInByte.begin());

  headerInByte[4] = header.version;
  headerInByte[5] = static_cast<std::uint8_t>(header.type);

  std::size_t offset{6};
  ConvertEndian::writeU16BE(headerInByte, offset, header.size);

  return headerInByte;
}

std::vector<std::uint8_t> serializeCommandPayload(
    const CommandPayload& payload) {
  ProtocolHelper::validateCommandPayload(payload);

  const std::size_t finalSize{COMMAND_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);
  std::size_t offset{0};
  // ConvertEndian::writeU16BE(payloadInByte, offset, payload.id);
  ConvertEndian::writeU32BE(payloadInByte, offset, payload.id);
  payloadInByte[offset] = static_cast<std::uint8_t>(payload.type);
  offset++;
  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  ProtocolHelper::copyString(payloadInByte, COMMAND_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeResponsePayload(
    const ResponsePayload& payload) {
  ProtocolHelper::validateResponsePayload(payload);

  const std::size_t finalSize{RESPONSE_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);
  std::size_t offset{0};
  // ConvertEndian::writeU16BE(payloadInByte, offset, payload.id);
  ConvertEndian::writeU32BE(payloadInByte, offset, payload.id);

  payloadInByte[offset] = static_cast<std::uint8_t>(payload.status);
  offset++;
  payloadInByte[offset] = static_cast<std::uint8_t>(payload.type);
  offset++;
  payloadInByte[offset] = payload.total_chunks;
  offset++;
  payloadInByte[offset] = payload.chunk_index;
  offset++;

  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  // copyString(payloadInByte, RESPONSE_FIXED_BYTES, payload.data);
  std::copy(payload.data.begin(), payload.data.end(),
            payloadInByte.begin() + RESPONSE_FIXED_BYTES);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeDataPayload(const DataPayload& payload) {
  ProtocolHelper::validateDataPayload(payload);

  const std::size_t finalSize{DATA_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.subtype);
  std::size_t offset{1};
  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.data.size()));
  // copyString(payloadInByte, DATA_FIXED_BYTES, payload.data);
  ConvertEndian::writeByteVector(payloadInByte, offset, payload.data);
  // std::copy(payload.data.begin(), payload.data.end(),
  //           payloadInByte.begin() + DATA_FIXED_BYTES);

  return payloadInByte;
}

std::vector<std::uint8_t> serializeErrorPayload(const ErrorPayload& payload) {
  ProtocolHelper::validateErrorPayload(payload);

  const std::size_t message_length{payload.message.size()};
  const std::size_t finalSize{ERROR_FIXED_BYTES + message_length};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.code);
  std::size_t offset{1};
  ConvertEndian::writeU16BE(payloadInByte, offset, message_length);

  ConvertEndian::writeString(payloadInByte, offset, payload.message);
  // ProtocolHelper::copyString(payloadInByte, ERROR_FIXED_BYTES,
  // payload.message);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeDashboardCommand(
    const DashboardCommand& command) {
  std::uint16_t target_len = command.target.size();
  std::uint16_t sent_at_len = command.sent_at.size();

  std::vector<uint8_t> commandPayload =
      serializeCommandPayload(command.command);

  std::size_t totalSize{2 * sizeof(std::uint16_t) + target_len + sent_at_len +
                        commandPayload.size()};

  std::vector<uint8_t> payload(totalSize);
  std::size_t offset{0};
  ConvertEndian::writeU16BE(payload, offset, target_len);
  ConvertEndian::writeU16BE(payload, offset, sent_at_len);
  ConvertEndian::writeString(payload, offset, command.target);
  // std::copy(command.target.begin(), command.target.end(),
  //           payload.begin() + offset);
  // offset += target_len;

  ConvertEndian::writeString(payload, offset, command.sent_at);
  // std::copy(command.sent_at.begin(), command.sent_at.end(),
  //           payload.begin() + offset);
  // offset += sent_at_len;
  ConvertEndian::writeByteVector(payload, offset, commandPayload);
  // std::copy(commandPayload.begin(), commandPayload.end(),
  //           payload.begin() + offset);
  return payload;
}

std::vector<std::uint8_t> serializeDashboardData(const DashboardData& data) {
  std::uint16_t target_len = data.target.size();
  std::vector<uint8_t> dataPayload = serializeDataPayload(data.data);

  std::size_t totalSize{sizeof(std::uint16_t) + target_len +
                        dataPayload.size()};
  std::vector<std::uint8_t> payload(totalSize);
  std::size_t offset{0};

  ConvertEndian::writeU16BE(payload, offset, target_len);
  ConvertEndian::writeString(payload, offset, data.target);
  // std::copy(data.target.begin(), data.target.end(), payload.begin() +
  // offset); offset += target_len;
  ConvertEndian::writeByteVector(payload, offset, dataPayload);
  // std::copy(dataPayload.begin(), dataPayload.end(), payload.begin() +
  // offset);

  return payload;
}

std::vector<std::uint8_t> serializeDashboardResponse(
    const DashboardResponse& response) {
  std::uint16_t target_len = response.target.size();
  std::uint16_t received_at_len = response.received_at.size();

  std::vector<uint8_t> responsePayload =
      serializeResponsePayload(response.response);

  std::size_t totalSize{2 * sizeof(std::uint16_t) + target_len +
                        received_at_len + responsePayload.size()};
  std::vector<std::uint8_t> payload(totalSize);
  std::size_t offset{0};

  ConvertEndian::writeU16BE(payload, offset, target_len);
  ConvertEndian::writeU16BE(payload, offset, received_at_len);

  // std::copy(response.target.begin(), response.target.end(),
  //           payload.begin() + offset);
  // offset += target_len;
  ConvertEndian::writeString(payload, offset, response.target);

  // std::copy(response.received_at.begin(), response.received_at.end(),
  //           payload.begin() + offset);
  // offset += received_at_len;
  ConvertEndian::writeString(payload, offset, response.received_at);

  // std::copy(responsePayload.begin(), responsePayload.end(),
  //           payload.begin() + offset);
  ConvertEndian::writeByteVector(payload, offset, responsePayload);

  return payload;
}

std::vector<std::uint8_t> serializeDashboardDisconnect(
    const DashboardDisconnect& payload) {
  std::uint16_t targetLen = payload.target.size();
  std::size_t totalSize{sizeof(std::uint16_t) + targetLen};
  std::vector<std::uint8_t> finalPayload(totalSize);

  std::size_t offset{0};
  ConvertEndian::writeU16BE(finalPayload, offset, targetLen);

  ConvertEndian::writeString(finalPayload, offset, payload.target);
  // std::copy(payload.target.begin(), payload.target.end(),
  //           finalPayload.begin() + offset);

  return finalPayload;
}

std::vector<std::uint8_t> serializeFrame(const Frame& frame) {
  if (frame.payload.size() > MAX_VALUE_INT16) {
    throw InvalidSize("payload", std::to_string(frame.payload.size()));
  }

  const std::vector<std::uint8_t> headerBytes = serializeHeader(frame.header);

  std::vector<uint8_t> frameBytes;
  frameBytes.reserve(headerBytes.size() + frame.payload.size());
  frameBytes.insert(frameBytes.end(), headerBytes.begin(), headerBytes.end());
  frameBytes.insert(frameBytes.end(), frame.payload.begin(),
                    frame.payload.end());
  return frameBytes;
}
}  // namespace ProtocolSerializer