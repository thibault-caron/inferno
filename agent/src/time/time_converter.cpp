#include "time/time_converter.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace TimeConverter {

std::string timestamptzToIso(const std::string& timestamptz) {
  std::tm tm{};

  // Only the "YYYY-MM-DD HH:MM:SS" prefix is parsed; fractional seconds
  // and the timezone offset (already known to be UTC) are ignored.
  std::istringstream iss(timestamptz);
  iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

  if (iss.fail()) {
    return "";
  }

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buffer);
}

std::string isoToTimestamptz(const std::string& iso) {
  std::tm tm{};

  std::istringstream iss(iso);
  iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  if (iss.fail()) {
    return "";
  }

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S+00", &tm);
  return std::string(buffer);
}

std::string systemClockToIso(const std::chrono::system_clock::time_point& tp) {
  std::time_t time = std::chrono::system_clock::to_time_t(tp);

  char buffer[32];

#ifdef _WIN32
  std::tm tm{};
  gmtime_s(&tm, &time);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
#else
  std::tm tm{};
  gmtime_r(&time, &tm);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
#endif

  return std::string(buffer);
}

}  // namespace TimeConverter