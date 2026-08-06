#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "builders/metrics_builder.hpp"
#include "protocol/lptf_protocol.hpp"
#include "codec/metrics_parser.hpp"
#include "codec/metrics_serializer.hpp"

// ── CpuSample ──────────────────────────────────────────────────────────────

TEST(MetricsRoundTrip,
     should_preserve_cpu_sample_through_serialize_then_parse) {
  CpuSample input = MetricsBuilder::createCpuSample();
  std::size_t offset{0};

  const std::vector<std::uint8_t> bytes =
      MetricsSerializer::serializeCpuSample(input);
  const CpuSample result = MetricsParser::parseCpuSample(bytes, offset);

  EXPECT_FLOAT_EQ(result.total_percent, input.total_percent);
  ASSERT_EQ(result.per_core.size(), input.per_core.size());
  for (std::size_t i = 0; i < input.per_core.size(); ++i)
    EXPECT_FLOAT_EQ(result.per_core[i], input.per_core[i]);
}

// ── MemSample ──────────────────────────────────────────────────────────────

TEST(MetricsRoundTrip,
     should_preserve_mem_sample_through_serialize_then_parse) {
  MemSample input = MetricsBuilder::createMemSample();
  std::size_t offset{0};

  const std::vector<std::uint8_t> bytes =
      MetricsSerializer::serializeMemSample(input);
  const MemSample result = MetricsParser::parseMemSample(bytes, offset);

  EXPECT_EQ(result.phys_total, input.phys_total);
  EXPECT_EQ(result.phys_used, input.phys_used);
  EXPECT_EQ(result.phys_available, input.phys_available);
  EXPECT_EQ(result.swap_total, input.swap_total);
  EXPECT_EQ(result.swap_used, input.swap_used);
}

// ── DiskSample ─────────────────────────────────────────────────────────────

TEST(MetricsRoundTrip,
     should_preserve_disk_sample_through_serialize_then_parse) {
  DiskSample input = MetricsBuilder::createDiskSample();
  std::size_t offset{0};

  const std::vector<std::uint8_t> bytes =
      MetricsSerializer::serializeDiskSample(input);
  const DiskSample result = MetricsParser::parseDiskSample(bytes, offset);

  EXPECT_EQ(result.read_bytes_per_sec, input.read_bytes_per_sec);
  EXPECT_EQ(result.write_bytes_per_sec, input.write_bytes_per_sec);
  EXPECT_EQ(result.device, input.device);
}

// ── NetSample ──────────────────────────────────────────────────────────────

TEST(MetricsRoundTrip,
     should_preserve_net_sample_through_serialize_then_parse) {
  NetSample input = MetricsBuilder::createNetSample();
  std::size_t offset{0};

  const std::vector<std::uint8_t> bytes =
      MetricsSerializer::serializeNetSample(input);
  const NetSample result = MetricsParser::parseNetSample(bytes, offset);

  EXPECT_EQ(result.rx_bytes_per_sec, input.rx_bytes_per_sec);
  EXPECT_EQ(result.tx_bytes_per_sec, input.tx_bytes_per_sec);
  EXPECT_EQ(result.iface, input.iface);
}

// ── MetricsSample (full) ───────────────────────────────────────────────────

TEST(MetricsRoundTrip,
     should_preserve_metrics_sample_through_serialize_then_parse) {
  MetricsSample input = MetricsBuilder::createMetricsSample();

  const std::vector<std::uint8_t> bytes =
      MetricsSerializer::serializeMetricsSample(input);
  const MetricsSample result = MetricsParser::parseMetricsSample(bytes);

  // cpu
  EXPECT_FLOAT_EQ(result.cpu.total_percent, input.cpu.total_percent);
  ASSERT_EQ(result.cpu.per_core.size(), input.cpu.per_core.size());
  for (std::size_t i = 0; i < input.cpu.per_core.size(); ++i)
    EXPECT_FLOAT_EQ(result.cpu.per_core[i], input.cpu.per_core[i]);

  // mem
  EXPECT_EQ(result.mem.phys_total, input.mem.phys_total);
  EXPECT_EQ(result.mem.phys_available, input.mem.phys_available);
  EXPECT_EQ(result.mem.swap_used, input.mem.swap_used);

  // disks
  ASSERT_EQ(result.disks.size(), input.disks.size());
  EXPECT_EQ(result.disks[0].device, input.disks[0].device);
  EXPECT_EQ(result.disks[0].read_bytes_per_sec,
            input.disks[0].read_bytes_per_sec);
  EXPECT_EQ(result.disks[1].device, input.disks[1].device);

  // interfaces
  ASSERT_EQ(result.interfaces.size(), input.interfaces.size());
  EXPECT_EQ(result.interfaces[0].iface, input.interfaces[0].iface);
  EXPECT_EQ(result.interfaces[0].rx_bytes_per_sec,
            input.interfaces[0].rx_bytes_per_sec);
  EXPECT_EQ(result.interfaces[1].iface, input.interfaces[1].iface);
}