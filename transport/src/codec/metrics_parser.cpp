#include "codec/metrics_parser.hpp"

#include <cstddef>
#include <string>

#include "codec/convert_endian.hpp"
#include "exception/lptf_exception.hpp"

MemSample MetricsParser::parseMemSample(const std::vector<uint8_t>& input,
                                        std::size_t& offset) {
  MemSample mem;
  mem.phys_total = ConvertEndian::readU64BE(input, offset);
  mem.phys_used = ConvertEndian::readU64BE(input, offset);
  mem.phys_available = ConvertEndian::readU64BE(input, offset);
  mem.swap_total = ConvertEndian::readU64BE(input, offset);
  mem.swap_used = ConvertEndian::readU64BE(input, offset);
  return mem;
}

DiskSample MetricsParser::parseDiskSample(const std::vector<uint8_t>& input,
                                          std::size_t& offset) {
  DiskSample disk;
  disk.read_bytes_per_sec = ConvertEndian::readFloat(input, offset);
  disk.write_bytes_per_sec = ConvertEndian::readFloat(input, offset);
  std::uint16_t deviceLen = ConvertEndian::readU16BE(input, offset);
  disk.device =
      std::string(input.begin() + offset, input.begin() + offset + deviceLen);
  offset += deviceLen;
  return disk;
}

NetSample MetricsParser::parseNetSample(const std::vector<uint8_t>& input,
                                        std::size_t& offset) {
  NetSample net;
  net.rx_bytes_per_sec = ConvertEndian::readFloat(input, offset);
  net.tx_bytes_per_sec = ConvertEndian::readFloat(input, offset);
  std::uint16_t ifaceLen = ConvertEndian::readU16BE(input, offset);
  net.iface =
      std::string(input.begin() + offset, input.begin() + offset + ifaceLen);
  offset += ifaceLen;
  return net;
}

CpuSample MetricsParser::parseCpuSample(const std::vector<uint8_t>& input,
                                        std::size_t& offset) {
  CpuSample cpu;
  cpu.total_percent = ConvertEndian::readFloat(input, offset);
  std::uint8_t coreNumber = input[offset];
  offset++;
  for (std::uint8_t i{0}; i < coreNumber; ++i) {
    cpu.per_core.push_back(ConvertEndian::readFloat(input, offset));
  }
  return cpu;
}

MetricsSample MetricsParser::parseMetricsSample(
    const std::vector<uint8_t>& input) {
  MetricsSample sample;
  std::size_t offset{0};
  sample.cpu = MetricsParser::parseCpuSample(input, offset);
  sample.mem = MetricsParser::parseMemSample(input, offset);
  std::uint16_t timestampLen = ConvertEndian::readU16BE(input, offset);

    std::uint8_t diskCount = input[offset];
  offset++;
  std::uint8_t interfaceCount = input[offset];
  offset++;
  
  // timestamp
  sample.timestamp = ConvertEndian::getString(input, offset, timestampLen);
  // disks
  for (std::uint8_t i{0}; i < diskCount; ++i) {
    sample.disks.push_back(MetricsParser::parseDiskSample(input, offset));
  }

  // Net
  for (std::uint8_t i{0}; i < interfaceCount; ++i) {
    sample.interfaces.push_back(MetricsParser::parseNetSample(input, offset));
  }

  return sample;
}