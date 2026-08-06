#ifndef PROTOCOL_SYSTEM_INFO_HPP
#define PROTOCOL_SYSTEM_INFO_HPP
#include <cstdint>
#include <limits>
#include <string>

// constexpr std::size_t KMAX_U16_VALUE = 65535u;
constexpr std::size_t KMAX_U16_VALUE =
    std::numeric_limits<std::uint16_t>::max();  //  = 65535u;
constexpr std::uint16_t MAX_VALUE_INT16 =
    static_cast<std::uint16_t>(KMAX_U16_VALUE);

constexpr std::size_t OS_INFO_FIXED_BYTES =
    5 * sizeof(std::uint16_t) +
    2 * sizeof(std::uint8_t);  // hostname_len + os_version_len +
                               // current_user_len + ip_len + os_type + arch +
                               // mac len
constexpr std::size_t REGISTER_MAX_HOSTNAME_LEN =
    MAX_VALUE_INT16 - OS_INFO_FIXED_BYTES;

constexpr std::size_t REGISTER_FIXED_SIZE =
    3 * sizeof(std::uint16_t) + sizeof(std::uint8_t);
//  // target_len + registered_at_len + last_seen_len
// + boolean as uint8_t

constexpr std::size_t PROCESS_INFO_FIXED_SIZE =
    sizeof(std::uint32_t) + sizeof(float) + sizeof(std::uint64_t) +
    sizeof(std::uint16_t);

enum class OSType : std::uint8_t {
  WINDOWS = 0,
  LINUX = 1,
  MAC = 2,
  UNKNOWN  // must be the last one
};

enum class ArchType : std::uint8_t {
  X86 = 0,
  X64 = 1,
  ARM = 2,
  UNKNOWN  // must be the last one
};

struct OsInfoPayload {
  OSType os_type = OSType::UNKNOWN;
  ArchType arch = ArchType::UNKNOWN;
  std::string hostname = "";
  std::string os_version = "";    // new — "Ubuntu 22.04", "Windows 11"
  std::string current_user = "";  // new — getenv("USER") / GetUserName()
  std::string ip = "";
  std::string mac = "";

  bool operator==(const OsInfoPayload& other) const {
    return os_type == other.os_type && arch == other.arch &&
           hostname == other.hostname && os_version == other.os_version &&
           current_user == other.current_user && ip == other.ip &&
           mac == other.mac;
  }
};

struct RegisterPayload {
  bool online = false;
  std::string id = "";
  OsInfoPayload system;
  std::string registered_at = "";
  std::string last_seen = "";

  bool operator==(const RegisterPayload& other) const {
    return id == other.id && system == other.system &&
           registered_at == other.registered_at &&
           last_seen == other.last_seen && online == other.online;
  }
};

struct ProcessInfo {
  std::uint32_t pid = 0;  // kernel process id
  std::string name = "";  // process name (up to 15 chars, from
                          // /proc/[pid]/status, -name, GetProcessImageFileName)
  float cpu_percent = 0.0f;  // lifetime average CPU % (utime+stime / uptime)
  std::uint64_t mem_bytes =
      0;  // resident set size in kB (VmRSS from /proc/[pid]/status)

  bool operator==(const ProcessInfo& other) const {
    return pid == other.pid && name == other.name &&
           cpu_percent == other.cpu_percent && mem_bytes == other.mem_bytes;
  }
};

#endif