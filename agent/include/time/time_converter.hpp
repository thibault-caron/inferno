#ifndef TIME_CONVERTER_HPP
#define TIME_CONVERTER_HPP

#include <chrono>
#include <string>

// Conversions between TIMESTAMPTZ (libpqxx) (e.g. "2026-08-03 10:15:30.123456+00") and ISO 8601 strings (e.g. "2026-08-03T10:15:30Z").
// Assume the database session runs in UTC
namespace TimeConverter {

// Returns an empty string if `timestamptz` cannot be parsed.
std::string timestamptzToIso(const std::string& timestamptz);

// Returns an empty string if `iso` cannot be parsed.
std::string isoToTimestamptz(const std::string& iso);

std::string systemClockToIso(const std::chrono::system_clock::time_point& tp);

}  // namespace TimeConverter

#endif
