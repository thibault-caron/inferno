#ifndef METRICS_BUILDER_HPP
#define METRICS_BUILDER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "protocol/lptf_protocol.hpp"

namespace MetricsBuilder {
inline CpuSample createCpuSample(float total_percent = 75.0f,
                                 std::vector<float> per_core = {10.0f, 20.0f,
                                                                30.0f, 40.0f}) {
  CpuSample cpu;
  cpu.total_percent = total_percent;
  cpu.per_core = per_core;
  return cpu;
}

inline MemSample createMemSample(std::uint64_t phys_total = 8000000000ULL,
                                 std::uint64_t phys_used = 4000000000ULL,
                                 std::uint64_t phys_available = 4000000000ULL,
                                 std::uint64_t swap_total = 2000000000ULL,
                                 std::uint64_t swap_used = 500000000ULL) {
  MemSample mem;
  mem.phys_total = phys_total;
  mem.phys_used = phys_used;
  mem.phys_available = phys_available;
  mem.swap_total = swap_total;
  mem.swap_used = swap_used;
  return mem;
}

inline DiskSample createDiskSample(
    std::uint64_t read_bytes_per_sec = 100000ULL,
    std::uint64_t write_bytes_per_sec = 200000ULL, std::string device = "sda") {
  DiskSample disk;
  disk.read_bytes_per_sec = read_bytes_per_sec;
  disk.write_bytes_per_sec = write_bytes_per_sec;
  disk.device = device;
  return disk;
}

inline NetSample createNetSample(std::uint64_t rx_bytes_per_sec = 500000ULL,
                                 std::uint64_t tx_bytes_per_sec = 250000ULL,
                                 std::string iface = "eth0") {
  NetSample net;
  net.rx_bytes_per_sec = rx_bytes_per_sec;
  net.tx_bytes_per_sec = tx_bytes_per_sec;
  net.iface = iface;
  return net;
}

inline MetricsSample createMetricsSample(int diskCount = 2,
                                         int ifaceCount = 2) {
  MetricsSample sample;
  sample.cpu = MetricsBuilder::createCpuSample();
  sample.mem = MetricsBuilder::createMemSample();
  sample.timestamp = "2026-07-28T14:32:45Z";
  for (int i{0}; i < diskCount; ++i)
    sample.disks.push_back(MetricsBuilder::createDiskSample());
  for (int i{0}; i < ifaceCount; ++i)
    sample.interfaces.push_back(MetricsBuilder::createNetSample());
  return sample;
}
}  // namespace MetricsBuilder

#endif