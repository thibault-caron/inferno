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

 public:
  ProtocolParser() = delete;
  ProtocolParser(const ProtocolParser&) = delete;
  ProtocolParser& operator=(const ProtocolParser&) = delete;

  static LptfHeader parseHeader(const std::vector<uint8_t>& input);
  static RegisterPayload parseRegisterPayload(
      const std::vector<uint8_t>& input);
};

#endif