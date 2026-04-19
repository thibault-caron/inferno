#ifndef CONVERT_ENDIAN_HPP
#define CONVERT_ENDIAN_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

// TODO what's the difference between std::uint8_t and std::uint8_t OR
// std::size_t and std::size_t...etc
// std::uint8_t means “take uint8_t from namespace std”. Better.
// uint8_t without std:: relies on it being available in global namespace on your compiler/environment.
class ConvertEndian {
 public:
  ConvertEndian() = delete;
  ConvertEndian(const ConvertEndian&) = delete;
  ConvertEndian& operator=(const ConvertEndian&) = delete;

  static std::uint16_t readU16BE(const std::vector<std::uint8_t>& buffer,
                                 std::size_t offset) {
    // high byte = offset, low byte = offset + 1
    return static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
  };

  static void writeU16BE(std::vector<std::uint8_t>& buffer, std::size_t offset,
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
  }
};

#endif