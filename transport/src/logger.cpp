#include "logger.hpp"

#include <iostream>
#include <sstream>

namespace Color {
constexpr const char* RED = "\033[31m";
constexpr const char* GREEN = "\033[32m";
constexpr const char* YELLOW = "\033[1;33m";
constexpr const char* BLUE = "\033[34m";
constexpr const char* RESET = "\033[0m";
}  // namespace Color

void Logger::log(const Level level, std::string_view who,
                 std::string_view what) {

                  std::ostringstream base;

   base << "[" << who << "] : " << what << "\n";
  switch (level) {
    case Level::INFO:
      std::cout << Color::BLUE << "[INFO] " << Color::RESET << base.str();
      break;
    case Level::WARN:
      std::cout << Color::YELLOW << "[WARN] " << Color::RESET << base.str();
      break;
    case Level::ERR:
      std::cerr << Color::RED << "[ERROR] " << Color::RESET << base.str();
      break;
  }
}

void Logger::info(std::string_view who, std::string_view what) {
  log(Level::INFO, who, what);
}

void Logger::warn(std::string_view who, std::string_view what) {
  log(Level::WARN, who, what);
}

void Logger::error(std::string_view who, std::string_view what) {
  log(Level::ERR, who, what);
}