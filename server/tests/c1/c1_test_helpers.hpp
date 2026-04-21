#ifndef C1_TEST_HELPERS_HPP
#define C1_TEST_HELPERS_HPP

#include <cstdint>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"

inline std::vector<std::uint8_t> makeRegisterFrame(
    const std::string& hostname) {
  const RegisterPayload payload{
      OSType::LINUX,
      ArchType::X64,
      hostname,
  };

  const std::vector<std::uint8_t> payloadBytes =
      ProtocolSerializer::serializeRegisterPayload(payload);

  LptfHeader header{{'L', 'P', 'T', 'F'},
                    LPTF_VERSION,
                    MessageType::REGISTER,
                    static_cast<std::uint16_t>(payloadBytes.size())};

  const std::vector<std::uint8_t> headerBytes =
      ProtocolSerializer::serializeHeader(header);

  std::vector<std::uint8_t> frame;
  frame.reserve(headerBytes.size() + payloadBytes.size());
  frame.insert(frame.end(), headerBytes.begin(), headerBytes.end());
  frame.insert(frame.end(), payloadBytes.begin(), payloadBytes.end());
  return frame;
}

#endif