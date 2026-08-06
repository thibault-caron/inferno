#ifndef PROTOCOL_HEADER_HPP
#define PROTOCOL_HEADER_HPP
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

constexpr std::uint16_t SERVER_PORT = 8888;

constexpr std::uint8_t LPTF_VERSION = 1;

constexpr std::uint8_t LPTF_HEADER_SIZE =
    sizeof(std::uint8_t) * 6 +
    sizeof(std::uint16_t);  // identifier + version + type + size

constexpr std::array<char, 4> LPTF_IDENTIFIER = {'L', 'P', 'T', 'F'};
constexpr std::string_view LPTF_IDENTIFIER_STR(LPTF_IDENTIFIER.data(), 4);
constexpr std::string_view DASHBORD_IDENTIFIER = "DASH";
enum class MessageType : std::uint8_t {
  REGISTER = 0,
  DASHBOARD_REGISTER = 1,
  DATA = 2,
  COMMAND = 3,
  RESPONSE = 4,
  DISCONNECT = 5,  // target name
  INFERNO_ERROR = 6,
  UNKNOWN  // must always be the last !!
};

struct LptfHeader {
  // char identifier[4];
  std::array<char, 4> identifier = LPTF_IDENTIFIER;
  std::uint8_t version = LPTF_VERSION;
  MessageType type = MessageType::UNKNOWN;
  std::uint16_t size = LPTF_HEADER_SIZE;

  bool operator==(const LptfHeader& other) const {
    return std::equal(std::begin(identifier), std::end(identifier),
                      std::begin(other.identifier)) &&
           version == other.version && type == other.type && size == other.size;
    // return identifier == other.identifier && version == other.version &&
    //        type == other.type && size == other.size;
  }
};

struct DashboardDisconnect {
  std::string target = "";  // "agent-1" to disconnect that agent
  bool operator==(const DashboardDisconnect& other) const {
    return target == other.target;
  }
};

// Generic struct for recv()
struct Frame {
  LptfHeader header;
  std::vector<std::uint8_t> payload = {};

  bool operator==(const Frame& other) const {
    return header == other.header && payload == other.payload;
  }
};

#endif