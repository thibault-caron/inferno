#ifndef LPTF_PARSER_HPP
#define LPTF_PARSER_HPP

// #include <cstring>

#include <cstdint>
#include <vector>

// #include "convert_endian.hpp"
// #include "exception/lptf_exception.hpp"
#include "protocol/lptf_protocol.hpp"

class ProtocolParser {
 public:
  ProtocolParser() = delete;
  ProtocolParser(const ProtocolParser&) = delete;
  ProtocolParser& operator=(const ProtocolParser&) = delete;

  static LptfHeader parseHeader(const std::vector<std::uint8_t>& input);
  static RegisterPayload parseRegisterPayload(
      const std::vector<std::uint8_t>& input);
  static CommandPayload parseCommandPayload(
      const std::vector<std::uint8_t>& input);
  static ResponsePayload parseResponsePayload(
      const std::vector<std::uint8_t>& input);
  static DataPayload parseDataPayload(const std::vector<std::uint8_t>& input);
  static ErrorPayload parseErrorPayload(const std::vector<std::uint8_t>& input);
};

#endif