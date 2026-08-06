#ifndef LINUX_METRICS_SCRAPPER_HPP
#define LINUX_METRICS_SCRAPPER_HPP

#include <chrono>
#include <map>

#include "metrics/i_metrics_scrapper.hpp"

class LinuxMetricsScrapper : public IMetricsScrapper {
 public:
  LinuxMetricsScrapper(std::string procRoot = "/proc");
  MetricsSample sample() override;

 private:
  std::string procRoot_;
  // delta state
  RawCpuSnapshot previousCpu_;
  std::map<std::string, RawDiskSnapshot> previousDisks_;
  std::map<std::string, RawNetSnapshot> previousNet_;
  bool firstSample_{true};
  // std::chrono::steady_clock::time_point lastSampleTime_;
  std::chrono::system_clock::time_point lastSampleTime_;  // ← Changed

  // per-subsystem private readers
  CpuSample readCpu();
  MemSample readMem();
  std::vector<DiskSample> readDisks(const float& elapsed);
  std::vector<NetSample> readNet(const float& elapsed);
  std::ifstream openFile(const std::string& relativePath);
  float computeCpuPercent(const RawCpuSnapshot& snapshot,
                          const std::size_t& index);
  RawCpuSnapshot getRawCpuSnapshot();
  std::map<std::string, RawNetSnapshot> getRawNetSnapshots();
  void cleaniFaceName(std::string& ifaceName);
  void skipUnusedValues(std::istringstream& lineStream, const int index);
  std::map<std::string, RawDiskSnapshot> getRawDiskSnapshots();
};

#endif