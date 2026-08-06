#ifndef I_SYSTEM_MONITOR_HPP
#define I_SYSTEM_MONITOR_HPP

#include <cstdint>

#include "protocol/lptf_protocol.hpp"

class ISystemMonitor {
 public:
  virtual ~ISystemMonitor() = default;
  virtual OsInfoPayload getOsInfo() = 0;
  virtual std::vector<ProcessInfo> getProcessList() = 0;
  // virtual MetricsSample sampleMetrics()                     = 0;
  virtual std::string executeShell(const std::string& command) = 0;
};

#endif  // I_SYSTEM_MONITOR_HPP