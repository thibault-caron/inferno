#ifndef CONVERT_ENDIAN_HPP
#define CONVERT_ENDIAN_HPP

#include <cstdint>
#include <vector>

class ConvertEndian {
 public:
  ConvertEndian() = delete;
  ConvertEndian(const ConvertEndian&) = delete;
  ConvertEndian& operator=(const ConvertEndian&) = delete;

  static uint16_t readU16BE(const std::vector<uint8_t>& buffer, size_t offset) {
    // high byte = offset, low byte = offset + 1
    return static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
  };
};

#endif