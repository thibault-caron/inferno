#include "metrics/linux_metrics_scrapper.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "logger.hpp"

std::ifstream LinuxMetricsScrapper::openFile(const std::string& relativePath) {
  std::ifstream file(procRoot_ + "/" + relativePath);
  if (!file) {
    throw std::runtime_error("Failed to open" + relativePath);
  }
  return file;
}

void LinuxMetricsScrapper::cleaniFaceName(std::string& ifaceName) {
  if (!ifaceName.empty() && ifaceName.back() == ':') {
    ifaceName.pop_back();
  }
}

void LinuxMetricsScrapper::skipUnusedValues(std::istringstream& lineStream,
                                            const int index) {
  std::uint64_t ignoredValue;
  for (int i{0}; i < index; ++i) {
    lineStream >> ignoredValue;
  }
}

LinuxMetricsScrapper::LinuxMetricsScrapper(std::string procRoot)
    : procRoot_(procRoot), firstSample_{true} {}

MemSample LinuxMetricsScrapper::readMem() {
  std::ifstream file = openFile("meminfo");
  MemSample mem{};

  std::string line;
  std::string key;
  std::uint64_t value;

  std::uint64_t swapFree{0};
  while (std::getline(file, line)) {
    std::istringstream lineStream(line);
    lineStream >> key >> value;

    // all value are in kiloBytes, * 1024 convert them back to bytes
    if (key == "MemTotal:") {
      mem.phys_total = value * 1024;
    } else if (key == "MemAvailable:") {
      mem.phys_available = value * 1024;
    } else if (key == "SwapTotal:") {
      mem.swap_total = value * 1024;
    } else if (key == "SwapFree:") {
      swapFree = value * 1024;
    }
  }

  mem.phys_used = mem.phys_total - mem.phys_available;
  mem.swap_used = mem.swap_total - swapFree;

  return mem;
}

CpuSample LinuxMetricsScrapper::readCpu() {
  CpuSample cpu{};
  RawCpuSnapshot snapshot = getRawCpuSnapshot();

  if (firstSample_) {
    previousCpu_ = snapshot;
    // firstSample_ = false; // should be done by sample() method!
    return cpu;  // first snapshot returns 0
  }

  // Guard against empty snapshots
  if (snapshot.total.empty() || previousCpu_.total.empty()) {
    return cpu;
  }

  cpu.total_percent = computeCpuPercent(snapshot, 0);

  for (std::size_t i{1}; i < snapshot.total.size(); ++i) {
    cpu.per_core.push_back(computeCpuPercent(snapshot, i));
  }
  previousCpu_ = snapshot;
  return cpu;
}

float LinuxMetricsScrapper::computeCpuPercent(const RawCpuSnapshot& snapshot,
                                              const std::size_t& index) {
  std::uint64_t deltTotal = snapshot.total[index] - previousCpu_.total[index];
  std::uint64_t deltaIdle = snapshot.idle[index] - previousCpu_.idle[index];
  if (deltTotal == 0) {
    return 0.0f;
  }
  return (1.0f -
          static_cast<float>(deltaIdle) / static_cast<float>(deltTotal)) *
         100.0f;
}

RawCpuSnapshot LinuxMetricsScrapper::getRawCpuSnapshot() {
  std::ifstream file = openFile("stat");
  RawCpuSnapshot snapshot;
  std::string line;
  std::string key;
  std::uint64_t user;
  std::uint64_t nice;
  std::uint64_t system;
  std::uint64_t idle;
  std::uint64_t iowait;
  std::uint64_t irq;
  std::uint64_t softirq;
  std::uint64_t steal;
  std::uint64_t guest;
  std::uint64_t guest_nice;

  while (std::getline(file, line)) {
    if (line.substr(0, 3) == "cpu") {
      std::istringstream lineStream(line);
      lineStream >> key >> user >> nice >> system >> idle >> iowait >> irq >>
          softirq >> steal >> guest >> guest_nice;

      std::uint64_t total{user + nice + system + idle + iowait + irq + softirq +
                          steal + guest + guest_nice};

      snapshot.total.push_back(total);
      snapshot.idle.push_back(idle);
    }
  }

  return snapshot;
}

std::map<std::string, RawNetSnapshot>
LinuxMetricsScrapper::getRawNetSnapshots() {
  std::ifstream file = openFile("net/dev");
  std::map<std::string, RawNetSnapshot> snapshot;
  std::string line;

  // std::uint8_t lineCount{0};
  while (std::getline(file, line)) {
    // lineCount++;
    // if (lineCount >= 2) {
    std::istringstream lineStream(line);
    skipUnusedValues(lineStream, 2);
    std::string ifaceName;
    lineStream >> ifaceName;
    cleaniFaceName(ifaceName);

    std::uint64_t receivedBytes;
    std::uint64_t transmittedBytes;
    lineStream >> receivedBytes;
    skipUnusedValues(lineStream, 8);
    lineStream >> transmittedBytes;

    snapshot[ifaceName] = {receivedBytes, transmittedBytes};
    // }
  }
  return snapshot;
}

std::vector<NetSample> LinuxMetricsScrapper::readNet(const float& elapsed) {
  std::vector<NetSample> result;
  std::map<std::string, RawNetSnapshot> currentNet = getRawNetSnapshots();
  if (firstSample_ || previousNet_.empty()) {
    previousNet_ = currentNet;
    return result;
  }

  for (const auto& [ifaceName, currentSnapshot] : currentNet) {
    if (previousNet_.find(ifaceName) != previousNet_.end()) {
      const RawNetSnapshot& previousSnapshot = previousNet_[ifaceName];

      std::uint64_t received_delta =
          currentSnapshot.rx_bytes - previousSnapshot.rx_bytes;
      std::uint64_t transmitted_delta =
          currentSnapshot.tx_bytes - previousSnapshot.tx_bytes;

      NetSample net;
      net.iface = ifaceName;
      net.rx_bytes_per_sec = static_cast<float>(received_delta) / elapsed;
      net.tx_bytes_per_sec = static_cast<float>(transmitted_delta) / elapsed;
      result.push_back(net);
    }
  }
  previousNet_ = currentNet;
  return result;
}

std::map<std::string, RawDiskSnapshot>
LinuxMetricsScrapper::getRawDiskSnapshots() {
  std::ifstream file = openFile("diskstats");
  std::map<std::string, RawDiskSnapshot> snapshots;
  std::string line;

  while (std::getline(file, line)) {
    // lineCount++;
    // if (lineCount >= 2) {
    std::istringstream lineStream(line);
    skipUnusedValues(lineStream, 2);
    std::string device;
    lineStream >> device;
    skipUnusedValues(lineStream, 3);

    std::uint64_t read_sectors;
    lineStream >> read_sectors;
    std::uint64_t read_bytes = read_sectors * 512;  // a sector is 512 bytes

    skipUnusedValues(lineStream, 4);
    std::uint64_t write_sectors;
    lineStream >> write_sectors;
    std::uint64_t write_bytes = write_sectors * 512;

    snapshots[device] = {read_bytes, write_bytes};
  }
  return snapshots;
}

std::vector<DiskSample> LinuxMetricsScrapper::readDisks(const float& elapsed) {
  std::vector<DiskSample> result;
  std::map<std::string, RawDiskSnapshot> currentDisks = getRawDiskSnapshots();

  if (firstSample_ || previousDisks_.empty()) {
    previousDisks_ = currentDisks;
    return result;
  }

  for (const auto& [device, currentSnapshot] : currentDisks) {
    if (previousDisks_.find(device) != previousDisks_.end()) {
      const RawDiskSnapshot& previousSnapshot = previousDisks_[device];

      std::uint64_t read_delta =
          currentSnapshot.read_bytes - previousSnapshot.read_bytes;
      std::uint64_t write_delta =
          currentSnapshot.write_bytes - previousSnapshot.write_bytes;

      DiskSample disk;
      disk.device = device;
      disk.read_bytes_per_sec = static_cast<float>(read_delta) / elapsed;
      disk.write_bytes_per_sec = static_cast<float>(write_delta) / elapsed;

      result.push_back(disk);
    }
  }
  previousDisks_ = currentDisks;
  return result;
}

MetricsSample LinuxMetricsScrapper::sample() {
  // Logger logger("LinuxMetricsScrapper");
  std::ostringstream what;
  MetricsSample sample;
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);

  std::ostringstream ss;
  ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
  sample.timestamp = ss.str();

  float elapsed = 0.0f;

  if (!firstSample_) {
    elapsed = std::chrono::duration<float>(now - lastSampleTime_).count();
  }

  try {
    sample.cpu = readCpu();
  } catch (const std::exception& e) {
    what << "readCpu() failed: " << e.what() << std::endl;
    Logger::error("LinuxMetricsScrapper", what.str());
  }

  try {
    sample.mem = readMem();
  } catch (const std::exception& e) {
    what << "readMem() failed: " << e.what() << std::endl;
    Logger::error("LinuxMetricsScrapper", what.str());
  }

  try {
    sample.disks = readDisks(elapsed);
  } catch (const std::exception& e) {
    what << "readDisks() failed: " << e.what() << std::endl;
    Logger::error("LinuxMetricsScrapper", what.str());
  }

  try {
    sample.interfaces = readNet(elapsed);
  } catch (const std::exception& e) {
    what << "readNet() failed: " << e.what() << std::endl;
    Logger::error("LinuxMetricsScrapper", what.str());
  }

  lastSampleTime_ = now;

  if (firstSample_) {
    firstSample_ = false;
  }

  return sample;
}