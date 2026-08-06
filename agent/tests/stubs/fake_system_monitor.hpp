#ifndef FAKE_SYSTEM_MONITOR_HPP
#define FAKE_SYSTEM_MONITOR_HPP

#include "builders/os_info_builder.hpp"
#include "builders/process_builder.hpp"
#include "fixtures/protocol.hpp"
#include "system_monitor/i_system_monitor.hpp"

class FakeSystemMonitor : public ISystemMonitor {
 public:
  std::uint32_t processCount = 3;
  std::string cannedShellOutput = "";
  std::string lastShellCommand = "";
  OsInfoPayload getOsInfo() override { return OsInfoBuilder::create(); }

  std::vector<ProcessInfo> getProcessList() override {
    return ProcessBuilder::createProcessInfoList(processCount);
    // ProcessInfo process1 =
    //     ProcessBuilder::createProcessInfo(1001, 12.5f, 123456, "proc-a");
    // ProcessInfo process2 =
    //     ProcessBuilder::createProcessInfo(1002, 7.25f, 654321, "proc-b");
    // return {process1, process2};
  }

  std::string executeShell(const std::string& cmd) override {
     lastShellCommand = cmd;
   
    return cannedShellOutput;
  }
};

#endif