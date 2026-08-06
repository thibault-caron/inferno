#ifndef PROTOCOL_ERROR_HPP
#define PROTOCOL_ERROR_HPP
#include <cstdint>
#include <string>

constexpr std::size_t ERROR_FIXED_BYTES =
    sizeof(std::uint16_t) + sizeof(std::uint8_t);

enum class ErrorType : std::uint8_t {
  UNKNOWN_TYPE = 0,
  INVALID_FORMAT = 1,
  UNKNOWN_COMMAND = 2,
  EXECUTION_FAILED = 3,
  SIZE_EXCEEDED = 4,
  INVALID_TYPE = 5,
  NOT_IMPLEMENTED = 6,
  UNKNOWN  // must be the last one
};

struct ErrorPayload {
  ErrorType code = ErrorType::UNKNOWN;
  std::string message = "";

  bool operator==(const ErrorPayload& other) const {
    return code == other.code && message == other.message;
  }
};

#endif