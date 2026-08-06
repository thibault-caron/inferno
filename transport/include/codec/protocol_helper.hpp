#ifndef PROTOCOL_HELPER_HPP
#define PROTOCOL_HELPER_HPP

// #include <iostream>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "protocol/lptf_protocol.hpp"

namespace ProtocolHelper {

void copyString(std::vector<std::uint8_t>& out, std::size_t offset,
                const std::string& value);

const char* messageTypeToString(MessageType type) noexcept;

LptfHeader createHeader(MessageType type,
                        const std::vector<std::uint8_t>& payload);

const std::size_t kReceiveChunkSize = 4096;

void validateNotNullLength(const std::uint16_t length,
                           const std::size_t maxLen);

void validateExpectedLength(const std::vector<std::uint8_t>& input,
                            const std::size_t expectedSize);

void validateStringLength(const std::uint16_t length,
                          const std::vector<std::uint8_t>& input,
                          const std::size_t maxLen,
                          const std::size_t expectedSize);

void validateChunkFields(const std::uint8_t totalChunks,
                         const std::uint8_t chunkIndex);

void ensureFitsU16(std::size_t sourceSize, const std::string& source);

void validateHeader(const LptfHeader& header);

void validateOsInfoPayload(const OsInfoPayload& payload);

void validateCommandPayload(const CommandPayload& payload);

void validateResponsePayload(const ResponsePayload& payload);
void validateDataPayload(const DataPayload& payload);
void validateErrorPayload(const ErrorPayload& payload);

void validateProcessInfo(const ProcessInfo& payload);

namespace EnumConversion {
OSType toOsType(const std::uint8_t value);

ArchType toArchType(const std::uint8_t value);

DataType toDataType(const std::uint8_t value);
CommandType toCommandType(const std::uint8_t value);

ResponseStatus toResponseStatus(const std::uint8_t value);

ErrorType toErrorType(const std::uint8_t value);

MessageType toMessageType(const std::uint8_t value);

}  // namespace EnumConversion
};  // namespace ProtocolHelper

#endif