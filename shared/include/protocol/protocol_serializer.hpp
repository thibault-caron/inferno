#ifndef LPTF_PARSER
#define LPTF_PARSER

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "convert_endian.hpp"
#include "exception/lptf_exception.hpp"
#include "protocol/lptf_protocol.hpp"

class ProtocolSerializer {
 public:
  ProtocolSerializer() = delete;
  ProtocolSerializer(const ProtocolSerializer&) = delete;
  ProtocolSerializer& operator=(const ProtocolSerializer&) = delete;

  static std::vector<uint8_t> serializeHeader(LptfHeader header);
};

#endif