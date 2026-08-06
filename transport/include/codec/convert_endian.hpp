#ifndef CONVERT_ENDIAN_HPP
#define CONVERT_ENDIAN_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// INFO what's the difference between std::uint8_t and uint8_t OR
// std::size_t and std::size_t...etc
// std::uint8_t means “take uint8_t from namespace std”. Better.
// uint8_t without std:: relies on it being available in global namespace on
// your compiler/environment.
namespace ConvertEndian {
//  public:
//   ConvertEndian() = delete;
//   ConvertEndian(const ConvertEndian&) = delete;
//   ConvertEndian& operator=(const ConvertEndian&) = delete;

// static std::uint16_t readU16BE(const std::vector<std::uint8_t>& buffer,
//                                std::size_t offset) {
//   // high byte = offset, low byte = offset + 1
//   return static_cast<std::uint16_t>(buffer[offset] << 8 | buffer[offset +
//   1]);
// };

std::uint16_t readU16BE(const std::vector<std::uint8_t>& buffer,
                        std::size_t& offset);

void writeU16BE(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                std::uint16_t value);

std::uint32_t readU32BE(const std::vector<std::uint8_t>& buffer,
                        std::size_t& offset);

std::uint64_t readU64BE(const std::vector<std::uint8_t>& buffer,
                        std::size_t& offset);

float readFloat(const std::vector<std::uint8_t>& buffer, std::size_t& offset);

std::string getString(const std::vector<std::uint8_t>& buffer,
                      std::size_t& offset, std::uint16_t length);
void writeString(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                 const std::string& value);
                 
void writeByteVector(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                     const std::vector<std::uint8_t>& value);

void writeU32BE(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                std::uint32_t value);

void writeU64BE(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                std::uint64_t value);

void writeFloat(std::vector<std::uint8_t>& buffer, std::size_t& offset,
                float value);
};  // namespace ConvertEndian

#endif