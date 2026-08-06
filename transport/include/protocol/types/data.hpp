#ifndef PROTOCOL_DATA_HPP
#define PROTOCOL_DATA_HPP
#include <cstdint>
#include <string>
#include <vector>

constexpr std::size_t DATA_FIXED_BYTES =
    sizeof(std::uint16_t) + sizeof(std::uint8_t);

enum class DataType : std::uint8_t {
  METRICS_SAMPLE = 0,
  HEALTH_CHECK = 1,
  REGISTRATION = 2,
  AGENTS = 3, // registerPayload list
  UNKNOWN  // must be the last one
};

struct DataPayload {
  DataType subtype = DataType::UNKNOWN;
  std::vector<std::uint8_t> data = {};

  bool operator==(const DataPayload& other) const {
    return subtype == other.subtype && data == other.data;
  }
};

struct DashboardData {
  std::string target;
  DataPayload data;

  bool operator==(const DashboardData& other) const {
    return target == other.target && data == other.data;
  }
};

#endif