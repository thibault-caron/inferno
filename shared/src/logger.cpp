#include "logger.hpp"

#include <iostream>


namespace Color {
    constexpr const char* RED   = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[1;33m";
    constexpr const char* BLUE  = "\033[34m";
    constexpr const char* RESET = "\033[0m";
}


void Logger::log(const Level level, const std::string& what) const {
  const std::string base = "[" + who_ + "] : " + what + "\n";
  switch (level) {
    case Level::INFO:
      std::cout << Color::BLUE << "[INFO] " << Color::RESET << base;
      break;
    case Level::WARN:
      std::cout << Color::YELLOW << "[WARN] " << Color::RESET << base;
      break;
    case Level::ERROR:
      std::cerr << Color::RED << "[ERROR] " << Color::RESET + base;
      break;
  }
}

void Logger::info(const std::string& what) const { log(Level::INFO, what); }

void Logger::warn(const std::string& what) const { log(Level::WARN, what); }

void Logger::error(const std::string& what) const { log(Level::ERROR, what); }