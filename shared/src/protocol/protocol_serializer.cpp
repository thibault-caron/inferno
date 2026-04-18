#include "protocol/protocol_serializer.hpp"

#include <cstddef>

namespace {

// constexpr std::size_t kMaxU16Value = 65535u;

void ensureFitsU16(std::size_t sourceSize, const std::string& source) {
  if (sourceSize > KMAX_U16_VALUE) {
    throw InvalidSize(source, std::to_string(sourceSize));
  }
}

void validateHeader(const LptfHeader& header) {
  const std::string inputIdentifier(header.identifier,
                                    sizeof(header.identifier));
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(inputIdentifier);
  }
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  if (header.type >= MessageType::END) {
    throw InvalidType(
        std::to_string(static_cast<std::uint8_t>(header.type)));
  }
}

// TODO check existing method to copy/convert a string into a vector LOOK FOR .insert(iterator.end, iterator.begin, destination?) method
void copyString(std::vector<std::uint8_t>& out, std::size_t offset,
                const std::string& value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    out[offset + i] = static_cast<std::uint8_t>(value[i]);
  }
}

void validateRegisterPayload(const RegisterPayload& payload) {
  if (payload.os_type >= OSType::END) {
    throw InvalidFieldValue(
        "os_type", std::to_string(static_cast<std::uint8_t>(payload.os_type)));
  }
  if (payload.arch >= ArchType::END) {
    throw InvalidFieldValue(
        "arch", std::to_string(static_cast<std::uint8_t>(payload.arch)));
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
  if (payload.type >= CommandType::END) {
    throw InvalidFieldValue(
        "command_type",
        std::to_string(static_cast<std::uint8_t>(payload.type)));
  }

  ensureFitsU16(payload.data.size(), "command data length");

  if (payload.type == CommandType::SHELL && payload.data.empty()) {
    throw InvalidFieldValue(
        "Data payload empty for a shell command, payload.data content : ",
        payload.data);
  }

  if (payload.type != CommandType::SHELL && !payload.data.empty()) {
    throw InvalidSize("command data length",
                      std::to_string(payload.data.size()));
  }
}

void validateResponsePayload(const ResponsePayload& payload) {
  ensureFitsU16(payload.data.size(), "response data length");

  if (payload.status >= ResponseStatus::END) {
    throw InvalidFieldValue(
        "response_status",
        std::to_string(static_cast<std::uint8_t>(payload.status)));
  }
  if (payload.total_chunks == 0) {
    throw InvalidFieldValue("total_chunks", "0");
  }
  if (payload.chunk_index >= payload.total_chunks) {
    throw InvalidFieldValue(
        "chunk_index",
        std::to_string(static_cast<std::uint8_t>(payload.chunk_index)));
  }
}

void validateDataPayload(const DataPayload& payload) {
  if (payload.subtype >= DataType::END) {
    throw InvalidFieldValue(
        "data_type",
        std::to_string(static_cast<std::uint8_t>(payload.subtype)));
  }
  ensureFitsU16(payload.data.size(), "data length");
}

void validateErrorPayload(const ErrorPayload& payload) {
  if (payload.code >= ErrorType::END) {
    throw InvalidFieldValue(
        "error_code", std::to_string(static_cast<std::uint8_t>(payload.code)));
  }
  if (payload.message.empty()) {
    throw InvalidSize("error message length", "0");
  }
  ensureFitsU16(payload.message.size(), "error message length");
}
}  // namespace

std::vector<std::uint8_t> ProtocolSerializer::serializeHeader(
    const LptfHeader& header) {
  validateHeader(header);
  
  std::vector<std::uint8_t> headerInByte(LPTF_HEADER_SIZE);

  for (std::size_t i = 0; i < sizeof(header.identifier); ++i) {
    headerInByte[i] = header.identifier[i];
  }

  headerInByte[4] = header.version;
  headerInByte[5] = static_cast<std::uint8_t>(header.type);

  ConvertEndian::writeU16BE(headerInByte, 6, header.size);

  return headerInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeRegisterPayload(
    const RegisterPayload& payload) {
  validateRegisterPayload(payload);

  const std::size_t finalSize{REGISTER_FIXED_BYTES + payload.hostname.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.os_type);
  payloadInByte[1] = static_cast<std::uint8_t>(payload.arch);
  ConvertEndian::writeU16BE(
      payloadInByte, 2, static_cast<std::uint16_t>(payload.hostname.size()));
  copyString(payloadInByte, REGISTER_FIXED_BYTES, payload.hostname);
  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeCommandPayload(
    const CommandPayload& payload) {
  validateCommandPayload(payload);

  const std::size_t finalSize{COMMAND_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  ConvertEndian::writeU16BE(payloadInByte, 0, payload.id);
  payloadInByte[2] = static_cast<std::uint8_t>(payload.type);
  ConvertEndian::writeU16BE(payloadInByte, 3,
                            static_cast<std::uint16_t>(payload.data.size()));
  copyString(payloadInByte, COMMAND_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeResponsePayload(
    const ResponsePayload& payload) {
  validateResponsePayload(payload);

  const std::size_t finalSize{RESPONSE_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  ConvertEndian::writeU16BE(payloadInByte, 0, payload.id);
  payloadInByte[2] = static_cast<std::uint8_t>(payload.status);
  payloadInByte[3] = payload.total_chunks;
  payloadInByte[4] = payload.chunk_index;
  ConvertEndian::writeU16BE(payloadInByte, 5,
                            static_cast<std::uint16_t>(payload.data.size()));
  copyString(payloadInByte, RESPONSE_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeDataPayload(
    const DataPayload& payload) {
  validateDataPayload(payload);

  const std::size_t finalSize{DATA_FIXED_BYTES + payload.data.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.subtype);
  ConvertEndian::writeU16BE(payloadInByte, 1,
                            static_cast<std::uint16_t>(payload.data.size()));
  copyString(payloadInByte, DATA_FIXED_BYTES, payload.data);
  return payloadInByte;
}

std::vector<std::uint8_t> ProtocolSerializer::serializeErrorPayload(
    const ErrorPayload& payload) {
  validateErrorPayload(payload);

  const std::size_t message_length{payload.message.size()};
  const std::size_t finalSize{ERROR_FIXED_BYTES + message_length};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.code);
  ConvertEndian::writeU16BE(payloadInByte, 1, message_length);

  copyString(payloadInByte, ERROR_FIXED_BYTES, payload.message);
  return payloadInByte;
}