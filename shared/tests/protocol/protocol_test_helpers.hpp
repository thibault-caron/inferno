#ifndef LPTF_PROTOCOL_TEST_HELPERS
#define LPTF_PROTOCOL_TEST_HELPERS

#include <cstdint>
#include <string>
#include <vector>

namespace TestHelpers {

constexpr uint8_t INVALID_ENUM_VALUE = 255;

inline void appendU16BE(std::vector<uint8_t>& out, uint16_t value) {
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(value & 0xFF));
}

inline std::vector<uint8_t> bytesFromString(const std::string& value) {
  return std::vector<uint8_t>(value.begin(), value.end());
}

}  // namespace TestHelpers

#endif
