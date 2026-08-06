#ifndef PROTOCOL_COMMAND_HPP
#define PROTOCOL_COMMAND_HPP
#include <cstdint>
#include <string>

constexpr std::size_t COMMAND_FIXED_BYTES =
    sizeof(std::uint32_t) + sizeof(std::uint16_t) +
    sizeof(std::uint8_t);  // id + type + data_len

enum class CommandType : std::uint8_t {
  OS_INFO = 0,
  RUNNING_PROCESSES = 1,
  SHELL = 2,
  START_METRICS = 3,
  STOP_METRICS = 4,
  UNKNOWN  // must be the last one
};

struct CommandPayload {
  std::uint32_t id = 0;
  CommandType type = CommandType::UNKNOWN;
  std::string data = "";

  bool operator==(const CommandPayload& other) const {
    return id == other.id && data == other.data && type == other.type;
  }
};

struct DashboardCommand {
  std::string target = "";
  CommandPayload command;
  // TODO adapt sent_at type + serializer & parser should create another
  // struct???
  std::string sent_at = "";

  bool operator==(const DashboardCommand& other) const {
    return target == other.target && command == other.command &&
           sent_at == other.sent_at;
  }
};
;

#endif
