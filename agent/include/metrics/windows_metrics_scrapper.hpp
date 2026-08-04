#ifndef WINDOWS_METRICS_SCRAPPER_HPP
#define WINDOWS_METRICS_SCRAPPER_HPP

#ifdef _WIN32

#ifndef WINVER
  #define WINVER 0x0600
#endif
#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN   // blocks wingdi.h → prevents ERROR macro clash with logger.hpp
#endif
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>  
#include <iphlpapi.h>   // GetIfTable / MIB_IFTABLE

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include "metrics/i_metrics_scrapper.hpp"

class WindowsMetricsScrapper : public IMetricsScrapper {
 public:
  WindowsMetricsScrapper();
  ~WindowsMetricsScrapper() override;

  WindowsMetricsScrapper(const WindowsMetricsScrapper&)            = delete;
  WindowsMetricsScrapper& operator=(const WindowsMetricsScrapper&) = delete;

  MetricsSample sample() override;

 private:
  // Helper type returned by getCounterItems()
  struct PdhItem {
    std::string name;
    double      value;
  };

  // Cumulative bytes snapshot for GetIfTable delta computation
  struct NetPrev {
    DWORD rx{0};
    DWORD tx{0};
  };

  bool firstSample_{true};
  std::chrono::system_clock::time_point lastSampleTime_;

  PDH_HQUERY   query_{nullptr};
  PDH_HCOUNTER cpuCounter_{nullptr};        // \Processor(*)\% Processor Time
  PDH_HCOUNTER diskReadCounter_{nullptr};   // \PhysicalDisk(*)\Disk Read Bytes/sec
  PDH_HCOUNTER diskWriteCounter_{nullptr};  // \PhysicalDisk(*)\Disk Write Bytes/sec

  std::unordered_map<DWORD, NetPrev> previousNet_;  // keyed by MIB interface index

  void                 initPdh();
  std::vector<PdhItem> getCounterItems(PDH_HCOUNTER counter);

  CpuSample               readCpu();
  MemSample               readMem();
  std::vector<DiskSample> readDisks();
  std::vector<NetSample>  readNet(float elapsed);
};

#endif  // _WIN32
#endif  // WINDOWS_METRICS_SCRAPPER_HPP