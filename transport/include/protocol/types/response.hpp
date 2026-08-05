#ifndef PROTOCOL_RESPONSE_HPP
#define PROTOCOL_RESPONSE_HPP
#include <cstdint>
#include <vector>

#include "command.hpp"

constexpr int MAC_SIZE = 17;

constexpr std::size_t RESPONSE_FIXED_BYTES =
    sizeof(std::uint32_t) + 2 * sizeof(std::uint16_t) +
    sizeof(std::uint8_t) * 4 +
    MAC_SIZE;  // id + data_len + received at len + status + total_chunks +
               // chunk_index + command type

enum class ResponseStatus : std::uint8_t {
  OK = 0,
  INFERNO_ERROR = 1,
  UNKNOWN  // must be the last one
};

struct ResponsePayload {
  std::uint32_t id = 0;
  ResponseStatus status = ResponseStatus::UNKNOWN;
  CommandType type = CommandType::UNKNOWN;
  std::uint8_t total_chunks = 0;
  std::uint8_t chunk_index = 0;
  std::vector<std::uint8_t> data = {};

  bool operator==(const ResponsePayload& other) const {
    return id == other.id && status == other.status && type == other.type &&
           total_chunks == other.total_chunks &&
           chunk_index == other.chunk_index && data == other.data;
  }
};

struct DashboardResponse {
  std::string target = "";  // which agent sent this response
  ResponsePayload response;
  // TODO adapt received_at type + serializer & parser
  std::string received_at = "";

  bool operator==(const DashboardResponse& other) const {
    return target == other.target && response == other.response &&
           received_at == other.received_at;
  }
};

#endif