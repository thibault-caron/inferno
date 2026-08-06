#include "codec/metrics_serializer.hpp"

#include <cstddef>
#include <string>

#include "codec/convert_endian.hpp"
#include "exception/lptf_exception.hpp"

std::vector<std::uint8_t> MetricsSerializer::serializeMemSample(
    const MemSample& sample) {
  std::vector<std::uint8_t> memSample(MEM_SAMPLE_FIXED_SIZE);
  std::size_t offset{0};
  ConvertEndian::writeU64BE(memSample, offset, sample.phys_total);
  ConvertEndian::writeU64BE(memSample, offset, sample.phys_used);
  ConvertEndian::writeU64BE(memSample, offset, sample.phys_available);
  ConvertEndian::writeU64BE(memSample, offset, sample.swap_total);
  ConvertEndian::writeU64BE(memSample, offset, sample.swap_used);
  return memSample;
}

std::vector<std::uint8_t> MetricsSerializer::serializeDiskSample(
    const DiskSample& sample) {
  std::vector<std::uint8_t> diskSample(DISK_SAMPLE_FIXED_SIZE +
                                       sample.device.size());
  std::size_t offset{0};
  ConvertEndian::writeFloat(diskSample, offset, sample.read_bytes_per_sec);
  ConvertEndian::writeFloat(diskSample, offset, sample.write_bytes_per_sec);
  ConvertEndian::writeU16BE(diskSample, offset, sample.device.size());
  // std::copy(sample.device.begin(), sample.device.end(),
  //           diskSample.begin() + offset);
  ConvertEndian::writeString(diskSample, offset, sample.device);
  return diskSample;
}

std::vector<std::uint8_t> MetricsSerializer::serializeNetSample(
    const NetSample& sample) {
  std::vector<std::uint8_t> netSample(NET_SAMPLE_FIXED_SIZE +
                                      sample.iface.size());
  std::size_t offset{0};
  ConvertEndian::writeFloat(netSample, offset, sample.rx_bytes_per_sec);
  ConvertEndian::writeFloat(netSample, offset, sample.tx_bytes_per_sec);
  ConvertEndian::writeU16BE(netSample, offset, sample.iface.size());
  // std::copy(sample.iface.begin(), sample.iface.end(),
  //           netSample.begin() + offset);
  ConvertEndian::writeString(netSample, offset, sample.iface);
  return netSample;
}

std::vector<std::uint8_t> MetricsSerializer::serializeCpuSample(
    const CpuSample& sample) {
  std::vector<std::uint8_t> cpuSample(CPU_SAMPLE_FIXED_SIZE +
                                      sizeof(float) * sample.per_core.size());
  std::size_t offset{0};

  ConvertEndian::writeFloat(cpuSample, offset, sample.total_percent);
  cpuSample[offset] = static_cast<std::uint8_t>(sample.per_core.size());
  offset++;

  for (const float& value : sample.per_core) {
    ConvertEndian::writeFloat(cpuSample, offset, value);
  }

  return cpuSample;
}

std::vector<std::uint8_t> MetricsSerializer::serializeMetricsSample(
    const MetricsSample& sample) {
  std::vector<std::uint8_t> metricsSample(
      MetricsSerializer::getMetricsSampleSize(sample));
  std::size_t offset{0};

  // cpu
  MetricsSerializer::serializeCpuSample(sample.cpu, metricsSample, offset);

  // mem
  MetricsSerializer::serializeMemSample(sample.mem, metricsSample, offset);

  ConvertEndian::writeU16BE(metricsSample, offset, sample.timestamp.size());

  metricsSample[offset] = static_cast<std::uint8_t>(sample.disks.size());
  offset++;
  metricsSample[offset] = static_cast<std::uint8_t>(sample.interfaces.size());
  offset++;
  // timestamp
  ConvertEndian::writeString(metricsSample, offset, sample.timestamp);
  // disk
  MetricsSerializer::serializeDiskSamples(sample, metricsSample, offset);

  // net
  MetricsSerializer::serializeNetSamples(sample, metricsSample, offset);

  return metricsSample;
}

void MetricsSerializer::serializeCpuSample(
    const CpuSample& cpuSample, std::vector<std::uint8_t>& metricsSample,
    std::size_t& offset) {
  std::vector<std::uint8_t> cpu =
      MetricsSerializer::serializeCpuSample(cpuSample);
  std::copy(cpu.begin(), cpu.end(), metricsSample.begin() + offset);
  offset += cpu.size();
}

void MetricsSerializer::serializeMemSample(
    const MemSample& memSample, std::vector<std::uint8_t>& metricsSample,
    std::size_t& offset) {
  std::vector<std::uint8_t> mem =
      MetricsSerializer::serializeMemSample(memSample);
  std::copy(mem.begin(), mem.end(), metricsSample.begin() + offset);
  offset += mem.size();
}

void MetricsSerializer::serializeDiskSamples(
    const MetricsSample& sample, std::vector<std::uint8_t>& metricsSample,
    std::size_t& offset) {
  for (const DiskSample& disk : sample.disks) {
    std::vector<std::uint8_t> diskSerialized =
        MetricsSerializer::serializeDiskSample(disk);
    std::copy(diskSerialized.begin(), diskSerialized.end(),
              metricsSample.begin() + offset);
    offset += diskSerialized.size();
  }
}

void MetricsSerializer::serializeNetSamples(
    const MetricsSample& sample, std::vector<std::uint8_t>& metricsSample,
    std::size_t& offset) {
  for (const NetSample& net : sample.interfaces) {
    std::vector<std::uint8_t> interface =
        MetricsSerializer::serializeNetSample(net);
    std::copy(interface.begin(), interface.end(),
              metricsSample.begin() + offset);
    offset += interface.size();
  }
}

std::size_t MetricsSerializer::getMetricsSampleSize(
    const MetricsSample& sample) {
  std::size_t totalSize{METRICS_SAMPLE_FIXED_SIZE};  // disk and interface count
                                                     // + timestamp_len value
  totalSize += sample.timestamp.size();  // reserve actual string length

  totalSize +=
      CPU_SAMPLE_FIXED_SIZE + (sample.cpu.per_core.size() * sizeof(float));
  totalSize += MEM_SAMPLE_FIXED_SIZE;

  for (const DiskSample& disk : sample.disks) {
    totalSize += DISK_SAMPLE_FIXED_SIZE + disk.device.size();
  }

  for (const NetSample& net : sample.interfaces) {
    totalSize += NET_SAMPLE_FIXED_SIZE + net.iface.size();
  }

  return totalSize;
}