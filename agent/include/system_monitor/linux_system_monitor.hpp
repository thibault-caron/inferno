#ifndef LINUX_SYSTEM_MONITOR_HPP
#define LINUX_SYSTEM_MONITOR_HPP

#include <dirent.h>

#include "system_monitor/i_system_monitor.hpp"

class LinuxSystemMonitor : public ISystemMonitor {
 public:
  LinuxSystemMonitor();
  ~LinuxSystemMonitor() override;

  OsInfoPayload getOsInfo() override;
  std::vector<ProcessInfo> getProcessList() override;
  // MetricsSample sampleMetrics()                     override;
  std::string executeShell(const std::string& command) override;

 private:
  // Reads the machine"s hostname ("inferno-agent-1")
  std::string readHostName();

  // read the current user's login name ("root")
  std::string readCurrentUser();

  // read the CPU architectture (X86_64 -> ArchType::X64)
  ArchType readArch();

  // read os vertion string (" Debian GNU/Linux 12")
  std::string readOsVersion();

  std::string readIpAddress();  // New method

  // double getUptime();
  // std::string readComm(int pid);
  // std::size_t readVmRss(int pid);
  // unsigned long long readCpuTicks(int pid);
  // float toCpuPercent(unsigned long long ticks, double uptime, long hz);

  std::string readMacAdress();

  ProcessInfo getProcessInfo(dirent* directoryEntry,
                             const double& systemUptimeSeconds,
                             const long& clockTicksPerSecond);
};

#endif  // LINUX_SYSTEM_MONITOR_HPP