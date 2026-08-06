#ifndef LPTF_METRICS_SERIALIZER_HPP
#define LPTF_METRICS_SERIALIZER_HPP

#include <cstdint>
#include <vector>

#include "protocol/lptf_protocol.hpp"

class MetricsSerializer {
 public:
  MetricsSerializer() = delete;
  MetricsSerializer(const MetricsSerializer&) = delete;
  MetricsSerializer& operator=(const MetricsSerializer&) = delete;

  static std::vector<std::uint8_t> serializeMetricsSample(
      const MetricsSample& sample);

  static std::size_t getMetricsSampleSize(const MetricsSample& sample);

  static std::vector<std::uint8_t> serializeMemSample(const MemSample& sample);
  static std::vector<std::uint8_t> serializeDiskSample(
      const DiskSample& sample);

  static std::vector<std::uint8_t> serializeCpuSample(const CpuSample& sample);

  static std::vector<std::uint8_t> serializeNetSample(const NetSample& sample);

 private:
  static void serializeDiskSamples(const MetricsSample& sample,
                                   std::vector<std::uint8_t>& metricsSample,
                                   std::size_t& offset);
  static void serializeNetSamples(const MetricsSample& sample,
                                  std::vector<std::uint8_t>& metricsSample,
                                  std::size_t& offset);

  static void serializeCpuSample(const CpuSample& cpuSample,
                                 std::vector<std::uint8_t>& metricsSample,
                                 std::size_t& offset);

  static void serializeMemSample(const MemSample& memSample,
                                 std::vector<std::uint8_t>& metricsSample,
                                 std::size_t& offset);
};

#endif