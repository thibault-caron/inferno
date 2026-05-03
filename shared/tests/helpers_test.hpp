#ifndef C1_TEST_HELPERS_HPP
#define C1_TEST_HELPERS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/socket_helper.hpp"

inline Frame makeFrame(MessageType type,
                       const std::vector<std::uint8_t>& payload = {}) {
  return {SocketHelper::createHeader(type, payload), payload};
}

// ── Raw frame builder ─────────────────────────────────────────
// Builds a complete wire-format frame (header + payload).
// Intentionally does NOT use ProtocolParser so that
// AgentSession tests are independent of the parser.
inline std::vector<std::uint8_t> makeRawFrame(
    MessageType type, const std::vector<std::uint8_t>& payload = {}) {
  const std::uint16_t size = static_cast<std::uint16_t>(payload.size());
  std::vector<std::uint8_t> frame = {
      'L',
      'P',
      'T',
      'F',
      LPTF_VERSION,
      static_cast<std::uint8_t>(type),
      static_cast<std::uint8_t>(size >> 8),
      static_cast<std::uint8_t>(size & 0xFF),
  };
  frame.insert(frame.end(), payload.begin(), payload.end());
  return frame;
}

// ── Typed payload builders (use serializer) ───────────────────

inline std::vector<std::uint8_t> makeRegisterPayload(
    const std::string& hostname, OSType os = OSType::LINUX,
    ArchType arch = ArchType::X64) {
  RegisterPayload p;
  p.os_type = os;
  p.arch = arch;
  p.hostname = hostname;
  return ProtocolSerializer::serializeRegisterPayload(p);
}

inline std::vector<std::uint8_t> makeResponsePayload(
    std::uint16_t id, const std::string& data, std::uint8_t totalChunks = 1,
    std::uint8_t chunkIndex = 0) {
  ResponsePayload p;
  p.id = id;
  p.status = ResponseStatus::OK;
  p.total_chunks = totalChunks;
  p.chunk_index = chunkIndex;
  p.data = data;
  return ProtocolSerializer::serializeResponsePayload(p);
}

#endif
