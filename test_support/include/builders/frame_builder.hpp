#ifndef FRAME_BUILDER_HPP
#define FRAME_BUILDER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"

namespace FrameBuilder {
inline Frame makeFrame(MessageType type,
                       const std::vector<std::uint8_t>& payload = {}) {
  return {ProtocolHelper::createHeader(type, payload), payload};
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

inline std::vector<std::uint8_t> makeRawFailingOsInfoPayload(
    const std::uint8_t rawOs = static_cast<uint8_t>(OSType::LINUX),
    const std::uint8_t rawArch = static_cast<uint8_t>(ArchType::X64),
    const std::uint16_t declaredHostnameLen = Protocol::TEST_HOSTNAME_LEN,
    const std::uint16_t declaredOsVersionLen = Protocol::TEST_OS_VERSION_LEN,
    const std::uint16_t declaredCurrentUserLen =
        Protocol::TEST_CURRENT_USER_LEN,
    const std::vector<std::uint8_t>& hostnameBytes = Protocol::TEST_HOSTNAME,
    const std::vector<std::uint8_t>& osVersionBytes = Protocol::TEST_OS_VERSION,
    const std::vector<std::uint8_t>& currentUserBytes =
        Protocol::TEST_CURRENT_USER,
    const std::uint16_t declaredIpLen = Protocol::TEST_IP_LEN,
    const std::vector<std::uint8_t>& ipBytes = Protocol::TEST_IP) {
  std::vector<std::uint8_t> out(OS_INFO_FIXED_BYTES);

  out[0] = rawOs;
  out[1] = rawArch;
  std::size_t offset{2};
  ConvertEndian::writeU16BE(out, offset, declaredHostnameLen);
  ConvertEndian::writeU16BE(out, offset, declaredOsVersionLen);
  ConvertEndian::writeU16BE(out, offset, declaredCurrentUserLen);
  ConvertEndian::writeU16BE(out, offset, declaredIpLen);

  out.insert(out.end(), hostnameBytes.begin(), hostnameBytes.end());
  out.insert(out.end(), osVersionBytes.begin(), osVersionBytes.end());
  out.insert(out.end(), currentUserBytes.begin(), currentUserBytes.end());
  out.insert(out.end(), ipBytes.begin(), ipBytes.end());
  return out;
}

inline OsInfoPayload makeOsInfoPayload(
    OSType os_type = OSType::LINUX, ArchType arch = ArchType::X64,
    const std::string& hostname = Protocol::TEST_HOSTNAME_STR,
    const std::string& os_version = Protocol::TEST_OS_VERSION_STR,
    const std::string& current_user = Protocol::TEST_CURRENT_USER_STR,
    const std::string& ip = Protocol::TEST_IP_STR,
    const std::string& mac = Protocol::TEST_MAC_STR) {
  OsInfoPayload info;
  info.os_type = os_type;
  info.arch = arch;
  info.hostname = hostname;
  info.os_version = os_version;
  info.current_user = current_user;
  info.ip = ip;
  info.mac = mac;
  return info;
}

inline std::vector<std::uint8_t> makeRawOsInfoPayload(
    OSType os_type = OSType::LINUX, ArchType arch = ArchType::X64,
    const std::string& hostname = Protocol::TEST_HOSTNAME_STR,
    const std::string& os_version = Protocol::TEST_OS_VERSION_STR,
    const std::string& current_user = Protocol::TEST_CURRENT_USER_STR,
    const std::string& ip = Protocol::TEST_IP_STR,
    const std::string& mac = Protocol::TEST_MAC_STR) {
  const OsInfoPayload info = {os_type,      arch, hostname, os_version,
                              current_user, ip,   mac};
  return ProtocolSerializer::serializeOsInfoPayload(info);
}

inline ResponsePayload makeResponsePayload(
    std::uint32_t id = 0, CommandType type = CommandType::OS_INFO,
    const std::string& data = "", std::uint8_t totalChunks = 1,
    std::uint8_t chunkIndex = 0) {
  ResponsePayload p;
  p.id = id;
  p.status = ResponseStatus::OK;
  p.type = type;
  p.total_chunks = totalChunks;
  p.chunk_index = chunkIndex;
  //   p.data = data;
  p.data.assign(data.begin(), data.end());

  // return ProtocolSerializer::serializeResponsePayload(p);
  return p;
}

// inline std::vector<std::uint8_t> makeRawResponsePayload(
//     std::uint32_t id = 0, const std::vector<std::uint8_t>& dataBytes = {}) {
//   std::vector<std::uint8_t> out(RESPONSE_FIXED_BYTES + dataBytes.size());
//   std::size_t offset{0};
//   ConvertEndian::writeU32BE(out, offset, id);
//   out[offset] = static_cast<std::uint8_t>(ResponseStatus::OK);
//   offset++;
//   out[offset] = static_cast<std::uint8_t>(CommandType::OS_INFO);
//   out[offset] = 1;
//   offset++;
//   out[offset] = 0;
//   offset++;
//   ConvertEndian::writeU16BE(out, offset, dataBytes.size());
//   std::copy(dataBytes.begin(), dataBytes.end(),
//             out.begin() + RESPONSE_FIXED_BYTES);
//   return out;
// }

inline std::vector<std::uint8_t> makeRawCommandPayload(
    std::uint32_t id, std::uint8_t rawType, std::uint16_t declaredLen,
    const std::vector<std::uint8_t>& dataBytes = {}) {
  std::vector<std::uint8_t> out(COMMAND_FIXED_BYTES + dataBytes.size());
  std::size_t offset{0};
  ConvertEndian::writeU32BE(out, offset, id);           // offset advances to 2
  out[offset] = rawType;                                // offset = 2
  offset++;                                             // offset = 3
  ConvertEndian::writeU16BE(out, offset, declaredLen);  // offset advances to 5
  std::copy(dataBytes.begin(), dataBytes.end(),
            out.begin() + COMMAND_FIXED_BYTES);
  return out;
}

inline CommandPayload makeCommandPayload(
    std::uint32_t id = 0, const CommandType& type = CommandType::OS_INFO,
    const std::string& dataBytes = "") {
  CommandPayload command;
  command.id = id;
  command.type = type;
  command.data = dataBytes;
  return command;
}

inline std::vector<std::uint8_t> makeRawCommandPayload(
    std::uint32_t id, std::uint8_t rawType,
    const std::vector<std::uint8_t>& dataBytes = {}) {
  std::vector<std::uint8_t> out(COMMAND_FIXED_BYTES + dataBytes.size());
  std::size_t offset{0};
  ConvertEndian::writeU32BE(out, offset, id);  // offset advances to 2
  out[offset] = rawType;                       // offset = 2
  offset++;                                    // offset = 3
  const std::uint16_t declaredLen =
      static_cast<std::uint16_t>(dataBytes.size());
  ConvertEndian::writeU16BE(out, offset, declaredLen);  // offset advances to 5
  std::copy(dataBytes.begin(), dataBytes.end(),
            out.begin() + COMMAND_FIXED_BYTES);
  return out;
}

inline std::vector<std::uint8_t> makeRawDataPayload(
    std::uint8_t rawSubtype, std::uint16_t declaredLen,
    const std::vector<std::uint8_t>& dataBytes = {}) {
  std::vector<std::uint8_t> out(DATA_FIXED_BYTES + dataBytes.size());
  out[0] = rawSubtype;
  std::size_t offset{1};
  ConvertEndian::writeU16BE(out, offset, declaredLen);
  std::copy(dataBytes.begin(), dataBytes.end(), out.begin() + DATA_FIXED_BYTES);
  return out;
}

inline DataPayload makeDataPayload(
    DataType subtype = DataType::METRICS_SAMPLE,
    const std::vector<std::uint8_t>& dataBytes = {}) {
  DataPayload data;
  data.subtype = subtype;
  data.data = dataBytes;
  return data;
}

inline std::vector<std::uint8_t> makeErrorPayload(
    std::uint8_t rawCode, std::uint16_t declaredLen,
    const std::vector<std::uint8_t>& messageBytes = {}) {
  std::vector<std::uint8_t> out(ERROR_FIXED_BYTES + messageBytes.size());
  out[0] = rawCode;
  std::size_t offset{1};
  ConvertEndian::writeU16BE(out, offset, declaredLen);
  std::copy(messageBytes.begin(), messageBytes.end(),
            out.begin() + ERROR_FIXED_BYTES);
  return out;
}

}  // namespace FrameBuilder

#endif