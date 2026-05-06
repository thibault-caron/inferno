#ifndef LPTF_SERIALIZER_HPP
#define LPTF_SERIALIZER_HPP

// #include <cstddef>
// #include <cstring>

#include <vector>

#include <cstdint>


#include "protocol/lptf_protocol.hpp"


#include "protocol/lptf_protocol.hpp"

class ProtocolSerializer {
 public:
  ProtocolSerializer() = delete;
  ProtocolSerializer(const ProtocolSerializer&) = delete;
  ProtocolSerializer& operator=(const ProtocolSerializer&) = delete;

  static std::vector<std::uint8_t> serializeHeader(const LptfHeader& header);
  static std::vector<std::uint8_t> serializeRegisterPayload(
      const RegisterPayload& payload);
  static std::vector<std::uint8_t> serializeCommandPayload(
      const CommandPayload& payload);
  static std::vector<std::uint8_t> serializeResponsePayload(
      const ResponsePayload& payload);
  static std::vector<std::uint8_t> serializeDataPayload(
      const DataPayload& payload);
  static std::vector<std::uint8_t> serializeErrorPayload(
      const ErrorPayload& payload);
};

#endif