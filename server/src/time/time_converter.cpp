#include "time/time_converter.hpp"

#include <cstdio>
#include <ctime>

namespace TimeConverter {

std::string timestamptzToIso(const std::string& timestamptz) {
  std::tm tm{};
  // Only the "YYYY-MM-DD HH:MM:SS" prefix is parsed; fractional seconds
  // and the timezone offset (already known to be UTC) are ignored.
  if (strptime(timestamptz.c_str(), "%Y-%m-%d %H:%M:%S", &tm) == nullptr) {
    return "";
  }

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buffer);
}

std::string isoToTimestamptz(const std::string& iso) {
  std::tm tm{};
  if (strptime(iso.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == nullptr) {
    return "";
  }

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S+00", &tm);
  return std::string(buffer);
}

std::string systemClockToIso(const std::chrono::system_clock::time_point& tp) {
  std::time_t time = std::chrono::system_clock::to_time_t(tp);

  // std::gmtime (not the _r/_s variant)
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time));
  return std::string(buffer);
}

}  // namespace TimeConverter
