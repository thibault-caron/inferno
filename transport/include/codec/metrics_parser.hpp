#ifndef LPTF_METRICS_PARSER_HPP
#define LPTF_METRICS_PARSER_HPP

#include <cstdint>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class MetricsParser {
 public:
  MetricsParser() = delete;
  MetricsParser(const MetricsParser&) = delete;
  MetricsParser& operator=(const MetricsParser&) = delete;

  static MemSample parseMemSample(const std::vector<uint8_t>& input,
                                  std::size_t& offset);
  static DiskSample parseDiskSample(const std::vector<uint8_t>& input,
                                    std::size_t& offset);
  static NetSample parseNetSample(const std::vector<uint8_t>& input,
                                  std::size_t& offset);
  static CpuSample parseCpuSample(const std::vector<uint8_t>& input,
                                  std::size_t& offset);
  static MetricsSample parseMetricsSample(const std::vector<uint8_t>& input);
  //  private:
};

#endif