#include "system_monitor/linux_system_monitor.hpp"

#include <arpa/inet.h>  // inet_ntop()
#include <ifaddrs.h>    // getifaddrs()
#include <net/if.h>
#include <pwd.h>
#include <sys/socket.h>   // address family constants
#include <sys/utsname.h>  // uname()
#include <unistd.h>       // gethostname()
#include <unistd.h>

#include <cctype>   // std::isdigit
#include <cstdio>   // popen(), pclose()
#include <cstdlib>  // getenv()
#include <fstream>  // std::ifstream
#include <sstream>
#include <string>
#include <vector>

#include "system_monitor/i_system_monitor.hpp"

LinuxSystemMonitor::LinuxSystemMonitor() {
  // Initialization code, if needed
}

LinuxSystemMonitor::~LinuxSystemMonitor() {
  // Cleanup code, if needed
}

// ── getProcessList helpers (file-local) ──────────────────────
// Each helper opens exactly one /proc file per process.
// If the file can't be opened (process vanished), it returns a
// safe default — the caller skips the entry when name is empty.

namespace {

// /proc/uptime  →  seconds since boot (first field)
double getSystemUptimeSeconds() {
  std::ifstream uptimeFile("/proc/uptime");
  double uptimeSeconds = 0.0;
  uptimeFile >> uptimeSeconds;
  return uptimeSeconds;
}

// /proc/[pid]/comm  →  executable name, kernel-limited to 15 chars
std::string readProcessCommName(uint32_t processId) {
  std::ifstream commFile("/proc/" + std::to_string(processId) + "/comm");
  std::string processName;
  std::getline(commFile, processName);
  return processName;  // empty = process vanished or kernel thread without comm
}

// /proc/[pid]/status  →  VmRSS line  →  resident memory in kB
std::size_t readProcessVmRssKb(int processId) {
  std::ifstream statusFile("/proc/" + std::to_string(processId) + "/status");
  std::string currentLine;
  while (std::getline(statusFile, currentLine)) {
    if (currentLine.compare(0, 6, "VmRSS:") == 0) {
      std::istringstream rssValueStream(currentLine.substr(6));
      std::size_t residentMemoryKb = 0;
      rssValueStream >> residentMemoryKb;
      return residentMemoryKb;
    }
  }
  return 0;  // kernel threads and zombie processes have no VmRSS
}

// /proc/[pid]/stat  →  utime + stime (fields 14 and 15, 1-indexed)
unsigned long long readProcessCpuTicks(uint32_t processId) {
  std::ifstream statFile("/proc/" + std::to_string(processId) + "/stat");
  std::string statLine;
  if (!std::getline(statFile, statLine)) return 0;

  const auto closingParenthesisPos = statLine.rfind(')');
  if (closingParenthesisPos == std::string::npos) return 0;

  std::istringstream fieldsStream(statLine.substr(closingParenthesisPos + 2));
  std::string skippedToken;
  for (int i = 0; i < 11 && fieldsStream >> skippedToken; ++i) {
  }  // skip state..cmajflt

  unsigned long long userModeTicks = 0, kernelModeTicks = 0;
  if (fieldsStream >> userModeTicks >> kernelModeTicks) {
    return userModeTicks + kernelModeTicks;
  }
  return 0;
}

// CPU lifetime average: what fraction of total uptime did this
// process consume? Multiply by 100 for a percentage.
// clockTicksPerSecond = CLK_TCK (typically 100 on Linux) converts jiffies to
// seconds.
float calculateCpuPercentage(unsigned long long totalProcessTicks,
                             double systemUptimeSeconds,
                             long clockTicksPerSecond) {
  if (systemUptimeSeconds <= 0.0 || clockTicksPerSecond <= 0) return 0.0f;
  return static_cast<float>(totalProcessTicks) /
         static_cast<float>(clockTicksPerSecond) /
         static_cast<float>(systemUptimeSeconds) * 100.0f;
}

// Removes leadling and trailing whitspace from a command string
std::string trim(const std::string& text) {
  const std::size_t first = text.find_first_not_of(" \t\n\r");

  if (first == std::string::npos) {
    return std::string{};  // empyt or whitespace-only command
  }

  const std::size_t last = text.find_last_not_of(" \t\n\r");
  return text.substr(first, last - first + 1);
}

// Commands starting with sudo are rejected to protect the agent's OS
bool startsWithSudo(const std::string& command) {
  return command.rfind("sudo", 0) == 0;  // true if sudo at the start
}

}  // namespace

OsInfoPayload LinuxSystemMonitor::getOsInfo() {
  OsInfoPayload info;

  info.os_type = OSType::LINUX;
  info.hostname = readHostName();
  info.current_user = readCurrentUser();
  info.arch = readArch();
  info.os_version = readOsVersion();
  info.ip = readIpAddress();
  info.mac = readMacAdress();

  return info;
}

// ── getProcessList ────────────────────────────────────────────
//
// Reads /proc at the moment of the call — the kernel maintains
// the virtual filesystem in real time, so the snapshot is current.
// Processes that die between readdir() and file open are skipped
// silently (readProcessCommName returns empty string).
std::vector<ProcessInfo> LinuxSystemMonitor::getProcessList() {
  const double systemUptimeSeconds = getSystemUptimeSeconds();
  const long clockTicksPerSecond = sysconf(_SC_CLK_TCK);

  std::vector<ProcessInfo> processListResult;

  DIR* procDirectory = opendir("/proc");
  if (!procDirectory) return processListResult;

  struct dirent* directoryEntry;
  while ((directoryEntry = readdir(procDirectory)) != nullptr) {
    // Only process directory entries that start with a digit (representing
    // PIDs)
    ProcessInfo info = getProcessInfo(directoryEntry, systemUptimeSeconds,
                                      clockTicksPerSecond);
    if (!info.name.empty()) {
      processListResult.push_back(info);
    }
  }
  closedir(procDirectory);
  return processListResult;
}

ProcessInfo LinuxSystemMonitor::getProcessInfo(
    dirent* directoryEntry, const double& systemUptimeSeconds,
    const long& clockTicksPerSecond) {
  ProcessInfo info;
  // Only process directory entries that start with a digit (representing
  // PIDs)
  if (std::isdigit(static_cast<unsigned char>(directoryEntry->d_name[0]))) {
    info.pid = static_cast<std::uint32_t>(std::stoi(directoryEntry->d_name));
    info.name = readProcessCommName(info.pid);

    // Ensure the process has not vanished (valid non-empty name)
    if (!info.name.empty()) {
      const auto totalProcessTicks = readProcessCpuTicks(info.pid);
      //   const float cpuUsagePercentage =
      info.cpu_percent = calculateCpuPercentage(
          totalProcessTicks, systemUptimeSeconds, clockTicksPerSecond);
      //   const std::size_t residentMemoryKb =
      info.mem_bytes = readProcessVmRssKb(info.pid);
    }
  }
  return info;
}

std::string LinuxSystemMonitor::executeShell(const std::string& command) {
  const std::string trimmed = trim(command);

  if (startsWithSudo(trimmed)) {
    return "Command rejected";
  }
  // Runs the command through /bin/sh - shell metacharacters are interpreted.
  // only sudo is rejected, so "ls ls; sudo rm -rf /" still runs the sudo part"
  // This is a guard rail, not a security boundary: access control belongs to
  // the server, which is the only TLS-authenticated peer allowed to send
  // commands.
  FILE* pipe = popen(trimmed.c_str(), "r");
  if (pipe == nullptr) {
    return std::string{};  // popen failed - nothing to read
  }
  char buffer[256];
  std::string output;

  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;  // Append the output to the result string
  }

  pclose(pipe);  // Close the pipe and get the exit status
  return output;
}

std::string LinuxSystemMonitor::readHostName() {
  char buffer[256];  // Linux hosnmaes ->255 chars

  // gethosname() fills the buffer and returns 0 on success  // -1 if error
  if (gethostname(buffer, sizeof(buffer)) != 0) {
    return std::string{};  // return empty string rather trhan crashing
  }

  return std::string{buffer};
}

std::string LinuxSystemMonitor::readCurrentUser() {
  // getenv() returns  a raw char* - or a nullptr if the variable is not set
  const char* user = getenv("USER");

  // USER is no always set - inside docker
  //  sy stem user db always knows  the current user

  if (user == nullptr) {
    const struct passwd* pw = getpwuid(getuid());
    if (pw != nullptr) {
      return std::string{pw->pw_name};  // ex root
    }
    return std::string{};
  }

  return std::string{user};
}

ArchType LinuxSystemMonitor::readArch() {
  // Todo : detect CPU architecture via uname()
  struct utsname info;

  // uname() fills the structure and returns 0(success) / -1 (error)
  if (uname(&info) != 0) {
    return ArchType::X64;  // fall backs to a senisible default on failure
  }

  std::string machine = info.machine;

  // TODO -> translate the machien string into an ArchType
  if (machine == "x86_64") {
    return ArchType::X64;
  } else if (machine == "i386" || machine == "i686") {
    return ArchType::X86;
  } else if ((machine.rfind("arm", 0) == 0) || (machine == "aarch64")) {
    return ArchType::ARM;
  }
  return ArchType::X64;  // default if the string is not recognized
}

std::string LinuxSystemMonitor::readOsVersion() {
  // TODO : read Pretty_NAME  form /etc/os-release/blabla

  std::ifstream file("/etc/os-release");

  if (!file.is_open()) {
    return std::string{};
    // address family constants
  }

  // TODO : read the file line by line and find PRETTY_NAME

  std::string line;

  // read file line by line
  while (std::getline(file, line)) {
    // TODO : check if this line stars with PRETTY_NAME= and extract it
    if (line.rfind("PRETTY_NAME=", 0) == 0) {
      // TOTO : extract the value after the '=' and strip the quote
      // cut off the "PRETTY_NAME=" to keep only the value
      std::string value = line.substr(std::string("PRETTY_NAME=").length());

      // the value is usually wrapped in double quotes - remove them
      if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
      }
      return value;
    }
  }

  return std::string{};
}

std::string LinuxSystemMonitor::readIpAddress() {
  struct ifaddrs* ifaddr = nullptr;
  std::string result;

  if (getifaddrs(&ifaddr) == -1) {
    throw std::runtime_error("Failed to retrieve network interfaces");
  }

  for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr != nullptr) {
      // Skip loopback and interfaces that aren't up
      if (!(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP)) {
        // Only handle IPv4 for now
        if (ifa->ifa_addr->sa_family == AF_INET) {
          struct sockaddr_in* ipv4 =
              reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);

          char addressBuffer[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &ipv4->sin_addr, addressBuffer, INET_ADDRSTRLEN);

          result = std::string{addressBuffer};
          freeifaddrs(ifaddr);

          return result;  // Return first valid non-loopback interface
        }
      }
    }
  }

  freeifaddrs(ifaddr);
  throw std::runtime_error("Failed to retrieve network interfaces");
  // return result;
}

std::string LinuxSystemMonitor::readMacAdress() {
  // This picks the same interface as readIpAddress() — first non-loopback IPv4
  // interface up — so IP and MAC always refer to the same NIC.
  struct ifaddrs* ifaddr = nullptr;

  if (getifaddrs(&ifaddr) == -1) {
    throw std::runtime_error("Failed to retrieve network interfaces");
  }

  for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) continue;
    if (ifa->ifa_flags & IFF_LOOPBACK) continue;
    if (!(ifa->ifa_flags & IFF_UP)) continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      // Found the same interface as readIpAddress()
      // Read MAC from /sys/class/net/<ifname>/address
      std::string macPath =
          "/sys/class/net/" + std::string(ifa->ifa_name) + "/address";
      std::ifstream macFile(macPath);

      if (macFile.is_open()) {
        std::string mac;
        std::getline(macFile, mac);
        freeifaddrs(ifaddr);
        return mac;  // e.g. "aa:bb:cc:dd:ee:ff"
      }
    }
  }

  freeifaddrs(ifaddr);
  throw std::runtime_error("Failed to retrieve MAC address");
}