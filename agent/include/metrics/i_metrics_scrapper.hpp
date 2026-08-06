#ifndef I_METRICS_SCRAPPER_HPP
#define I_METRICS_SCRAPPER_HPP

#include <cstdint>

#include "protocol/lptf_protocol.hpp"

// raw reading from /proc/stat — cumulative jiffies per core
struct RawCpuSnapshot {
    std::vector<uint64_t> total;  // total jiffies per core
    std::vector<uint64_t> idle;   // idle jiffies per core
};

// raw reading from /proc/net/dev — cumulative bytes
struct RawNetSnapshot {
    std::uint64_t rx_bytes;
    std::uint64_t tx_bytes;
};

// raw reading from /proc/diskstats — cumulative bytes
struct RawDiskSnapshot {
    std::uint64_t read_bytes;
    std::uint64_t write_bytes;
};

class IMetricsScrapper {
 public:
  virtual ~IMetricsScrapper() = default;
  virtual MetricsSample sample() = 0;
};

#endif