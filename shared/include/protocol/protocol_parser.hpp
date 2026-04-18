#ifndef LPTF_PARSER
#define LPTF_PARSER

#include <cstring>
#include <string>
#include <vector>

#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "protocol/lptf_protocol.hpp"

class ProtocolParser {
 private:
  static MessageType toMessageType(uint8_t value);
  static void validateStringLength(uint16_t length,
                                   const std::vector<uint8_t>& input,
                                   size_t MAX_LEN, size_t expectedSize);
  static void validateNotNullLength(uint16_t length, size_t MAX_LEN);

  static void validateExpectedLength(const std::vector<uint8_t>& input,
                                     size_t expectedSize);

 public:
  ProtocolParser() = delete;
  ProtocolParser(const ProtocolParser&) = delete;
  ProtocolParser& operator=(const ProtocolParser&) = delete;

  static LptfHeader parseHeader(const std::vector<uint8_t>& input);
  static RegisterPayload parseRegisterPayload(
      const std::vector<uint8_t>& input);
  static CommandPayload parseCommandPayload(const std::vector<uint8_t>& input);
  static ResponsePayload parseResponsePayload(
      const std::vector<uint8_t>& input);
  static DataPayload parseDataPayload(const std::vector<uint8_t>& input);
  static ErrorPayload parseErrorPayload(const std::vector<uint8_t>& input);
};

#endif