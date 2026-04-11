#ifndef CONVERT_ENDIAN_HPP
#define CONVERT_ENDIAN_HPP

#include <cstdint>
#include <cstddef>
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

  static void writeU16BE(std::vector<uint8_t>& buffer, size_t offset,
                         uint16_t value) {
    //  00000001 00101100 = 300 in big endian
                          
    uint8_t low = value & 0xFF;
    //   00000001 00101100
    // & 00000000 11111111
    //   00000000 00101100 // here is the least significant 

    uint8_t high = (value >> 8) & 0xFF;
    //   00000001 00101100 >> 8 => 00000000 00000001
    //   00000000 00000001
    // & 00000000 11111111
    //   00000000 00000001

    buffer[offset] = high;
    buffer[offset + 1] = low;
  }
};

#endif