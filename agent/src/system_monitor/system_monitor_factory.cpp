#include "system_monitor/system_monitor_factory.hpp"

std::unique_ptr<ISystemMonitor> SystemMonitorFactory::createSystemMonitor() {
#ifdef _WIN32
    return std::unique_ptr<ISystemMonitor>(new WindowsSystemMonitor());
#elif defined(__linux__)
    return std::unique_ptr<ISystemMonitor>(new LinuxSystemMonitor());
#elif defined(__APPLE__)
    return std::unique_ptr<ISystemMonitor>(new AppleSystemMonitor());
#endif
}