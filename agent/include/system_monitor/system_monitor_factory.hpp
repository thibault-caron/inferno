#ifndef SYSTEM_MONITOR_FACTORY_HPP
#define SYSTEM_MONITOR_FACTORY_HPP


#include <memory>

#ifdef _WIN32
#include "system_monitor/windows_system_monitor.hpp"
#elif defined(__linux__)
#include "system_monitor/linux_system_monitor.hpp"
#elif defined(__APPLE__)
#include "system_monitor/apple_system_monitor.hpp" // macOS is POSIX-compatible, same impl works
#endif

#include "system_monitor/i_system_monitor.hpp"
class SystemMonitorFactory {
public:
    static std::unique_ptr<ISystemMonitor> createSystemMonitor();
};

#endif