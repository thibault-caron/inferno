#include "protocol/protocol_parser.hpp"

namespace {
// TODO const for argument almost everywhere ?
OSType toOsType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(OSType::END)) {
    throw InvalidFieldValue("os_type", std::to_string(value));
  }
  return static_cast<OSType>(value);
}

ArchType toArchType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ArchType::END)) {
    throw InvalidFieldValue("arch", std::to_string(value));
  }
  return static_cast<ArchType>(value);
}

DataType toDataType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(DataType::END)) {
    throw InvalidFieldValue("data_type",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<DataType>(value);
}

CommandType toCommandType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(CommandType::END)) {
    throw InvalidFieldValue("command_type",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<CommandType>(value);
}

ResponseStatus toResponseStatus(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ResponseStatus::END)) {
    throw InvalidFieldValue("response_status",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<ResponseStatus>(value);
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

ErrorType toErrorType(const std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(ErrorType::END)) {
    throw InvalidFieldValue("error_code",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<ErrorType>(value);
}

MessageType toMessageType(const std::uint8_t value) {
  if (value > static_cast<std::uint8_t>(MessageType::ERROR)) {
    throw InvalidType(std::to_string(static_cast<std::uint8_t>(value)));
  }
  return static_cast<MessageType>(value);
}

// TODO  does copying std::uint16_t cost less than using a reference ? Const
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

}  // namespace

LptfHeader ProtocolParser::parseHeader(const std::vector<std::uint8_t>& input) {
  if (input.size() < 8) {
    throw InvalidSize(std::string("header"), std::to_string(input.size()));
  }

  const std::string_view inputIdentifier(
      reinterpret_cast<const char*>(input.data()), 4);
  if (inputIdentifier != LPTF_IDENTIFIER_STR) {
    throw InvalidIdentifier(std::string(inputIdentifier));
  }

  LptfHeader header;
  for (std::size_t i = 0; i < 4; ++i) {
    header.identifier[i] = static_cast<char>(input[i]);
  }

  header.version = input[4];
  if (header.version != LPTF_VERSION) {
    throw UnsupportedVersion(std::to_string(header.version),
                             "Version provided is not a number");
  }
  header.type = toMessageType(input[5]);
  header.size = ConvertEndian::readU16BE(input, 6);
  return header;
}

RegisterPayload ProtocolParser::parseRegisterPayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < REGISTER_FIXED_BYTES) {
    throw InvalidSize("register payload", std::to_string(input.size()));
  }
  const std::uint16_t hostnameLen{ConvertEndian::readU16BE(input, 2)};

  const std::size_t maxHostnameLength{MAX_VALUE_INT16 - REGISTER_FIXED_BYTES};
  const std::size_t expectedSize{REGISTER_FIXED_BYTES + hostnameLen};
  validateStringLength(hostnameLen, input, maxHostnameLength, expectedSize);

  RegisterPayload payload;
  payload.os_type = toOsType(input[0]);
  payload.arch = toArchType(input[1]);
  payload.hostname.assign(
      reinterpret_cast<const char*>(input.data() + REGISTER_FIXED_BYTES),
      hostnameLen);
  return payload;
}

DataPayload ProtocolParser::parseDataPayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < DATA_FIXED_BYTES) {
    throw InvalidSize("data payload", std::to_string(input.size()));
  }
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, 1)};
  const std::size_t expectedSize{DATA_FIXED_BYTES + dataLen};
  const std::size_t maxLength{MAX_VALUE_INT16 - DATA_FIXED_BYTES};

  validateStringLength(dataLen, input, maxLength, expectedSize);

  DataPayload payload;
  payload.subtype = toDataType(input[0]);
  payload.data.assign(
      reinterpret_cast<const char*>(input.data() + DATA_FIXED_BYTES), dataLen);
  return payload;
}

CommandPayload ProtocolParser::parseCommandPayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < COMMAND_FIXED_BYTES) {
    throw InvalidSize("command payload", std::to_string(input.size()));
  }

  const CommandType type = toCommandType(input[2]);
  const std::uint16_t dataLen = ConvertEndian::readU16BE(input, 3);

  const std::size_t expectedSize{COMMAND_FIXED_BYTES + dataLen};
  validateExpectedLength(input, expectedSize);

  CommandPayload payload;
  payload.id = ConvertEndian::readU16BE(input, 0);
  payload.type = type;

  if (payload.type == CommandType::SHELL && dataLen != 0) {
    payload.data.assign(
        reinterpret_cast<const char*>(input.data() + COMMAND_FIXED_BYTES),
        dataLen);
  }

  return payload;
}

ResponsePayload ProtocolParser::parseResponsePayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < RESPONSE_FIXED_BYTES) {
    throw InvalidSize("response payload", std::to_string(input.size()));
  }
  validateChunkFields(input[3], input[4]);
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, 5)};

  const std::size_t expectedSize{RESPONSE_FIXED_BYTES + dataLen};
  validateExpectedLength(input, expectedSize);

  ResponsePayload payload;
  payload.id = ConvertEndian::readU16BE(input, 0);
  payload.status = toResponseStatus(input[2]);
  payload.total_chunks = input[3];
  payload.chunk_index = input[4];
  payload.data.assign(
      reinterpret_cast<const char*>(input.data() + RESPONSE_FIXED_BYTES),
      dataLen);
  return payload;
}

ErrorPayload ProtocolParser::parseErrorPayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < ERROR_FIXED_BYTES) {
    throw InvalidSize("error payload", std::to_string(input.size()));
  }

  const std::uint16_t messageLen{ConvertEndian::readU16BE(input, 1)};
  const std::size_t expectedSize{ERROR_FIXED_BYTES + messageLen};
  const std::size_t maxLength{MAX_VALUE_INT16 - ERROR_FIXED_BYTES};

  // if (input.size() != expectedSize) {
  //   throw InvalidSize("error payload", std::to_string(input.size()));
  // }

  validateStringLength(messageLen, input, maxLength, expectedSize);

  ErrorPayload payload;
  payload.code = toErrorType(input[0]);
  payload.message.assign(
      reinterpret_cast<const char*>(input.data() + ERROR_FIXED_BYTES),
      messageLen);
  return payload;
}
