#include "codec/convert_endian.hpp"

#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#define be64toh(x) _byteswap_uint64(x)
#else
#include <arpa/inet.h>
#include <endian.h>
#endif

namespace ConvertEndian {
void writeU16BE(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                std::uint16_t value) {
  //  00000001 00101100 = 300 in big endian

  const std::uint8_t low = static_cast<std::uint8_t>(value & 0xFF);
  //   00000001 00101100
  // & 00000000 11111111
  //   00000000 00101100 // here is the least significant

  const std::uint8_t high = static_cast<std::uint8_t>((value >> 8) & 0xFF);
  //   00000001 00101100 >> 8 => 00000000 00000001
  //   00000000 00000001
  // & 00000000 11111111
  //   00000000 00000001

  buffer[offset] = high;
  buffer[offset + 1] = low;
  offset += sizeof(std::uint16_t);
}

// std::uint16_t readU16BE(const std::vector<std::uint8_t>&
// buffer,
//                                        std::size_t offset) {

//   return static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset +
//   1]);
// };

std::uint16_t readU16BE(const std::vector<std::uint8_t>& buffer,
                        std::size_t& offset) {  // offset by ref
  // high byte = offset, low byte = offset + 1
  std::uint16_t value =
      static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
  offset += sizeof(std::uint16_t);
  return value;
}

std::uint32_t readU32BE(const std::vector<std::uint8_t>& buffer,
                        std::size_t& offset) {
  uint32_t raw_value;
  std::memcpy(&raw_value, buffer.data() + offset, sizeof(raw_value));
  offset += sizeof(uint32_t);
  return ntohl(raw_value);
}

std::uint64_t readU64BE(const std::vector<std::uint8_t>& buffer,
                        std::size_t& offset) {
  uint64_t raw_mem;
  std::memcpy(&raw_mem, buffer.data() + offset, sizeof(raw_mem));
  offset += sizeof(uint64_t);
  return be64toh(raw_mem);
}

float readFloat(const std::vector<std::uint8_t>& buffer, std::size_t& offset) {
  float value;
  std::uint32_t raw_float = readU32BE(buffer, offset);
  std::memcpy(&value, &raw_float, sizeof(value));
  return value;
}

std::string getString(const std::vector<std::uint8_t>& buffer,
                      std::size_t& offset, std::uint16_t length) {
  std::string finalString = std::string(
      reinterpret_cast<const char*>(buffer.data() + offset), length);
  offset += length;
  return finalString;
}

void writeString(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                 const std::string& value) {
  std::copy(value.begin(), value.end(), buffer.begin() + offset);
  offset += value.size();
}

void writeByteVector(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                     const std::vector<std::uint8_t>& value) {
  std::copy(value.begin(), value.end(), buffer.begin() + offset);
  offset += value.size();
}
void writeU32BE(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                std::uint32_t value) {
  buffer[offset] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
  buffer[offset + 1] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
  buffer[offset + 2] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
  buffer[offset + 3] = static_cast<std::uint8_t>(value & 0xFF);
  offset += sizeof(std::uint32_t);
}

void writeU64BE(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                std::uint64_t value) {
  buffer[offset] = static_cast<std::uint8_t>((value >> 56) & 0xFF);
  buffer[offset + 1] = static_cast<std::uint8_t>((value >> 48) & 0xFF);
  buffer[offset + 2] = static_cast<std::uint8_t>((value >> 40) & 0xFF);
  buffer[offset + 3] = static_cast<std::uint8_t>((value >> 32) & 0xFF);
  buffer[offset + 4] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
  buffer[offset + 5] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
  buffer[offset + 6] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
  buffer[offset + 7] = static_cast<std::uint8_t>(value & 0xFF);
  offset += sizeof(std::uint64_t);
}

void writeFloat(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                float value) {
  std::uint32_t raw;
  std::memcpy(&raw, &value, sizeof(raw));
  writeU32BE(buffer, offset, raw);
}
}  // namespace ConvertEndian
