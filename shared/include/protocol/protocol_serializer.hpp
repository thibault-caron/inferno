#ifndef LPTF_SERIALIZER
#define LPTF_SERIALIZER

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

  static std::vector<uint8_t> serializeHeader(const LptfHeader& header);
  static std::vector<uint8_t> serializeErrorPayload(const ErrorPayload& payload);
};

#endif