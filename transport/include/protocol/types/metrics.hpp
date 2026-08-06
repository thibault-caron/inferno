#ifndef PROTOCOL_METRICS_HPP
#define PROTOCOL_METRICS_HPP
#include <cstdint>
#include <string>
#include <vector>
// CPU: float total + uint8_t count  (per_core is variable)
constexpr std::size_t CPU_SAMPLE_FIXED_SIZE =
    sizeof(float) + sizeof(std::uint8_t);

// MEM: 5 × uint64_t, fully fixed
constexpr std::size_t MEM_SAMPLE_FIXED_SIZE = 5 * sizeof(std::uint64_t);

// DISK: uint16_t device_len + 2 × uint64_t  (device string is variable)
constexpr std::size_t DISK_SAMPLE_FIXED_SIZE =
    sizeof(std::uint16_t) + 2 * sizeof(float);

// NET: uint16_t iface_len + 2 × uint64_t  (iface string is variable)
constexpr std::size_t NET_SAMPLE_FIXED_SIZE =
    sizeof(std::uint16_t) + 2 * sizeof(float);

// METRICS top-level: 2 × uint8_t for disk_count + interface_count
// (CpuSample and MemSample are inlined, variable themselves)
constexpr std::size_t METRICS_SAMPLE_FIXED_SIZE =
    sizeof(std::uint8_t) * 2 +
    sizeof(std::uint16_t);  // vector size + string length for timestamp

constexpr int METRICS_INTERVAL_MS = 1000;

struct CpuSample {
  float total_percent = 0.0f;
  std::vector<float> per_core;  // one per logical core

  bool operator==(const CpuSample& other) const {
    return total_percent == other.total_percent && per_core == other.per_core;
  }
};

struct MemSample {
  std::uint64_t phys_total = 0;
  std::uint64_t phys_used = 0;
  std::uint64_t phys_available = 0;
  std::uint64_t swap_total = 0;
  std::uint64_t swap_used = 0;

  bool operator==(const MemSample& other) const {
    return phys_total == other.phys_total && phys_used == other.phys_used &&
           phys_available == other.phys_available &&
           swap_total == other.swap_total && swap_used == other.swap_used;
  }
};

struct DiskSample {
  std::string device;  // e.g. "sda", "C:"
  float read_bytes_per_sec = 0.0f;
  float write_bytes_per_sec = 0.0f;

  bool operator==(const DiskSample& other) const {
    return device == other.device &&
           read_bytes_per_sec == other.read_bytes_per_sec &&
           write_bytes_per_sec == other.write_bytes_per_sec;
  }
};

struct NetSample {
  std::string iface;  // e.g. "eth0", "Ethernet"
  float rx_bytes_per_sec = 0.0f;
  float tx_bytes_per_sec = 0.0f;

  bool operator==(const NetSample& other) const {
    return iface == other.iface && rx_bytes_per_sec == other.rx_bytes_per_sec &&
           tx_bytes_per_sec == other.tx_bytes_per_sec;
  }
};

struct MetricsSample {
  std::string timestamp = "";
  CpuSample cpu;
  MemSample mem;
  std::vector<DiskSample> disks;
  std::vector<NetSample> interfaces;

  bool operator==(const MetricsSample& other) const {
    return timestamp == other.timestamp && cpu == other.cpu &&
           mem == other.mem && disks == other.disks &&
           interfaces == other.interfaces;
  }
};

#endif