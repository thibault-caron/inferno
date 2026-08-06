#include "codec/protocol_helper.hpp"

#include "exception/lptf_exception.hpp"
#include "logger.hpp"

namespace ProtocolHelper {
const char* messageTypeToString(MessageType type) noexcept {
  switch (type) {
    case MessageType::REGISTER:
      return "REGISTER";
    case MessageType::DASHBOARD_REGISTER:
      return "DASHBOARD REGISTER";
    case MessageType::DATA:
      return "DATA";
    case MessageType::COMMAND:
      return "COMMAND";
    case MessageType::RESPONSE:
      return "RESPONSE";
    case MessageType::DISCONNECT:
      return "DISCONNECT";
    case MessageType::INFERNO_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

LptfHeader createHeader(MessageType type,
                        const std::vector<std::uint8_t>& payload) {
  LptfHeader header{{'L', 'P', 'T', 'F'},
                    LPTF_VERSION,
                    type,
                    static_cast<std::uint16_t>(payload.size())};
  return header;
}

// INFO  does copying std::uint16_t cost less than using a reference ? Const
// anyway ?
// pass-by-value/copy generally better for small scalar types (uint8_t,
// uint16_t, int, enums, pointers...)
void validateNotNullLength(const std::uint16_t length,
                           const std::size_t maxLen) {
  if (length == 0 || length > maxLen) {
    throw InvalidSize("Struct string length", std::to_string(length));
  }
}

void validateExpectedLength(const std::vector<std::uint8_t>& input,
                            const std::size_t expectedSize) {
  if (input.size() != expectedSize) {
    throw InvalidSize("Payload", std::to_string(input.size()));
  }
}

void validateStringLength(const std::uint16_t length,
                          const std::vector<std::uint8_t>& input,
                          const std::size_t maxLen,
                          const std::size_t expectedSize) {
  validateNotNullLength(length, maxLen);
  validateExpectedLength(input, expectedSize);
}

void validateChunkFields(const std::uint8_t totalChunks,
                         const std::uint8_t chunkIndex) {
  if (totalChunks == 0) {
    throw InvalidFieldValue("total_chunks", "0");
  }
  if (chunkIndex >= totalChunks) {
    throw InvalidFieldValue(
        "chunk_index", std::to_string(static_cast<unsigned int>(chunkIndex)));
  }
}

void ensureFitsU16(std::size_t sourceSize, const std::string& source) {
  if (sourceSize > KMAX_U16_VALUE) {
    throw InvalidSize(source, std::to_string(sourceSize));
  }
}

void validateHeader(const LptfHeader& header) {
  const std::string inputIdentifier(header.identifier.data(),
                                    sizeof(header.identifier));
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(inputIdentifier);
  }
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  if (header.type >= MessageType::UNKNOWN) {
    throw InvalidType(std::to_string(static_cast<std::uint8_t>(header.type)));
  }
}

void copyString(std::vector<std::uint8_t>& out, std::size_t offset,
                const std::string& value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    out[offset + i] = static_cast<std::uint8_t>(value[i]);
  }
}

void validateOsInfoPayload(const OsInfoPayload& payload) {
  if (payload.os_type >= OSType::UNKNOWN) {
    throw InvalidFieldValue(
        "os_type", std::to_string(static_cast<std::uint8_t>(payload.os_type)));
  }
  if (payload.arch >= ArchType::UNKNOWN) {
    throw InvalidFieldValue(
        "architecture",
        std::to_string(static_cast<std::uint8_t>(payload.arch)));
  }
  if (payload.hostname.empty()) {
    throw InvalidSize("os info hostname length", "0");
  }

  if (payload.os_version.empty()) {
    throw InvalidSize("os info os_version length", "0");
  }

  if (payload.current_user.empty()) {
    throw InvalidSize("os info current_user length", "0");
  }

  if (payload.ip.empty()) {
    throw InvalidSize("os info ip length", "0");
  }

  if (payload.hostname.size() > REGISTER_MAX_HOSTNAME_LEN) {
    throw InvalidSize("os info hostname length",
                      std::to_string(payload.hostname.size()));
  }

  ensureFitsU16(payload.hostname.size(), "os info hostname length");

  ensureFitsU16(payload.os_version.size(), "os info os_version length");

  ensureFitsU16(payload.current_user.size(), "os info current_user length");

  ensureFitsU16(payload.ip.size(), "os info ip length");

  ensureFitsU16(payload.mac.size(), "os info mac length");
}

void validateCommandPayload(const CommandPayload& payload) {
  if (payload.type >= CommandType::UNKNOWN) {
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

  if (payload.status >= ResponseStatus::UNKNOWN) {
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
  if (payload.subtype >= DataType::UNKNOWN) {
    throw InvalidFieldValue(
        "data_type",
        std::to_string(static_cast<std::uint8_t>(payload.subtype)));
  }
  ensureFitsU16(payload.data.size(), "data length");
}

void validateErrorPayload(const ErrorPayload& payload) {
  if (payload.code >= ErrorType::UNKNOWN) {
    throw InvalidFieldValue(
        "error_code", std::to_string(static_cast<std::uint8_t>(payload.code)));
  }
  if (payload.message.empty()) {
    throw InvalidSize("error message length", "0");
  }
  ensureFitsU16(payload.message.size(), "error message length");
}

void validateProcessInfo(const ProcessInfo& payload) {
  if (payload.cpu_percent < 0.0f) {
    throw InvalidFieldValue(
        "cpu_percent", std::to_string(static_cast<float>(payload.cpu_percent)));
  }

  if (payload.name.empty()) {
    throw InvalidSize("error name length", "0");
  }
  ensureFitsU16(payload.name.size(), "error name length");
}

namespace EnumConversion {
OSType toOsType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(OSType::UNKNOWN)) {
    throw InvalidFieldValue("os_type", std::to_string(value));
  }
  return static_cast<OSType>(value);
}

ArchType toArchType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ArchType::UNKNOWN)) {
    throw InvalidFieldValue("architecture", std::to_string(value));
  }
  return static_cast<ArchType>(value);
}

DataType toDataType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(DataType::UNKNOWN)) {
    throw InvalidFieldValue("data_type",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<DataType>(value);
}

CommandType toCommandType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(CommandType::UNKNOWN)) {
    throw InvalidFieldValue("command_type",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<CommandType>(value);
}

ResponseStatus toResponseStatus(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ResponseStatus::UNKNOWN)) {
    throw InvalidFieldValue("response_status",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<ResponseStatus>(value);
}

ErrorType toErrorType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ErrorType::UNKNOWN)) {
    throw InvalidFieldValue("error_code",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<ErrorType>(value);
}

MessageType toMessageType(const std::uint8_t value) {
  if (value > static_cast<std::uint8_t>(MessageType::INFERNO_ERROR)) {
    throw InvalidType(std::to_string(static_cast<std::uint8_t>(value)));
  }
  return static_cast<MessageType>(value);
}

}  // namespace EnumConversion

}  // namespace ProtocolHelper