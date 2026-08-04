#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdint>
#include <string>
#include <string_view>

enum class Level : std::uint8_t { INFO, WARN, ERR };

namespace Logger {
  void log(const Level level, std::string_view who, std::string_view what);
  void info(std::string_view who, std::string_view what) ;
  void warn(std::string_view who, std::string_view what) ;
  void error(std::string_view who, std::string_view what) ;
};

#endif