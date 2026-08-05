#ifndef WINDOWS_SYSTEM_MONITOR_HPP
#define WINDOWS_SYSTEM_MONITOR_HPP

#ifdef _WIN32

#include <string>
#include <vector>

#include "system_monitor/i_system_monitor.hpp"

// All Windows SDK headers are confined to the .cpp to avoid polluting
// every translation unit that transitively includes this header.
class WindowsSystemMonitor : public ISystemMonitor {
 public:
  WindowsSystemMonitor()  = default;
  ~WindowsSystemMonitor() override = default;

  WindowsSystemMonitor(const WindowsSystemMonitor&)            = delete;
  WindowsSystemMonitor& operator=(const WindowsSystemMonitor&) = delete;

  OsInfoPayload            getOsInfo() override;
  std::vector<ProcessInfo> getProcessList() override;
  std::string              executeShell(const std::string& command) override;

 private:
  std::string readHostName();
  std::string readCurrentUser();
  ArchType    readArch();
  std::string readOsVersion();
  std::string readIpAddress();
  std::string readMacAddress();
};

#endif  // _WIN32
#endif  // WINDOWS_SYSTEM_MONITOR_HPP