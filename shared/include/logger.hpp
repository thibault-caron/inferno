#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <cstdint>

enum class Level : std::uint8_t {
  INFO,
  WARN,
  ERROR
};

class Logger {
 private:
  std::string who_;
  void log(const Level level, const std::string& what) const;

 public:
  explicit Logger(const std::string who) : who_(who) {}
  ~Logger() = default;
  Logger() = delete;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void info(const std::string& what) const;
  void warn(const std::string& what) const;
  void error(const std::string& what) const;
};

#endif