#include "logger.hpp"

#include <iostream>

void Logger::log(const Level level, const std::string& what) const {
  const std::string base = "[" + who_ + "] : " + what + "\n";
  switch (level) {
    case Level::INFO:
      std::cout << "[INFO] " + base;
      break;
    case Level::WARN:
      std::cout << "[WARN] " + base;
      break;
    case Level::ERROR:
      std::cerr << "[ERROR] " + base;
      break;
  }
}

void Logger::info(const std::string& what) const { log(Level::INFO, what); }

void Logger::warn(const std::string& what) const { log(Level::WARN, what); }

void Logger::error(const std::string& what) const { log(Level::ERROR, what); }