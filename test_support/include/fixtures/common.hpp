#ifndef COMMON_FIXTURE_HPP
#define COMMON_FIXTURE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace Common {
inline const std::string SERVER_HOST = "127.0.0.1";  // or localhost?
constexpr std::uint8_t INVALID_ENUM_VALUE = 255;

inline std::vector<std::uint8_t> bytesFromString(
    const std::string& value) {
  return std::vector<std::uint8_t>(value.begin(), value.end());
}

}  // namespace Common

namespace Tls {
inline const std::string SERVER_CERT = "certs/server.crt";
inline const std::string SERVER_KEY = "certs/server.key";
inline const std::string CA_CERT = "certs/ca.crt";
}  // namespace Tls

#endif