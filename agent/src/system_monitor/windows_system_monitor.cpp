#ifdef _WIN32

// winsock2.h MUST come before windows.h to avoid winsock redefinition errors.
#ifndef WINVER
  #define WINVER 0x0600
#endif
#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>    // inet_ntop
#include <windows.h>
#include <winternl.h>    // RTL_OSVERSIONINFOW / RtlGetVersion
#include <iphlpapi.h>    // GetAdaptersAddresses
#include <psapi.h>       // GetProcessMemoryInfo / PROCESS_MEMORY_COUNTERS
#include <tlhelp32.h>    // CreateToolhelp32Snapshot / PROCESSENTRY32
#include <lmcons.h>      // UNLEN

#include <cstring>       // strnlen
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "system_monitor/windows_system_monitor.hpp"

// ── getOsInfo ────────────────────────────────────────────────────────────────

OsInfoPayload WindowsSystemMonitor::getOsInfo() {
  OsInfoPayload info{};
  info.os_type      = OSType::WINDOWS;
  info.hostname     = readHostName();
  info.current_user = readCurrentUser();
  info.arch         = readArch();
  info.os_version   = readOsVersion();
  info.ip           = readIpAddress();
  info.mac          = readMacAddress();
  return info;
}

std::string WindowsSystemMonitor::readHostName() {
  DWORD size = 0;
  GetComputerNameExA(ComputerNameDnsHostname, nullptr, &size);
  if (size == 0) return {};

  std::string name(size, '\0');
  if (!GetComputerNameExA(ComputerNameDnsHostname, name.data(), &size)) return {};
  // size now holds the length without the null terminator
  name.resize(size);
  return name;
}

std::string WindowsSystemMonitor::readCurrentUser() {
  DWORD size = UNLEN + 1;
  std::string name(size, '\0');
  if (!GetUserNameA(name.data(), &size)) return {};
  name.resize(size > 0 ? size - 1 : 0);  // size includes the null terminator
  return name;
}

ArchType WindowsSystemMonitor::readArch() {
  SYSTEM_INFO si{};
  GetNativeSystemInfo(&si);
  switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return ArchType::X64;
    case PROCESSOR_ARCHITECTURE_INTEL: return ArchType::X86;
    case PROCESSOR_ARCHITECTURE_ARM:
    case PROCESSOR_ARCHITECTURE_ARM64: return ArchType::ARM;
    default:                            return ArchType::X64;  // safe fallback
  }
}

std::string WindowsSystemMonitor::readOsVersion() {
  // RtlGetVersion (ntdll.dll) bypasses the compatibility shim applied by
  // GetVersionEx, so it always returns the real OS version on Win10/11.
  using RtlGetVersionFn = LONG (WINAPI*)(RTL_OSVERSIONINFOW*);

  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  if (!ntdll) return "Windows";

  auto RtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
      reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
  if (!RtlGetVersion) return "Windows";

  RTL_OSVERSIONINFOW vi{};
  vi.dwOSVersionInfoSize = sizeof(vi);
  RtlGetVersion(&vi);

  std::string name;
  if (vi.dwMajorVersion == 10 && vi.dwMinorVersion == 0)
    name = (vi.dwBuildNumber >= 22000) ? "Windows 11" : "Windows 10";
  else
    name = "Windows " + std::to_string(vi.dwMajorVersion)
           + "." + std::to_string(vi.dwMinorVersion);

  return name + " (Build " + std::to_string(vi.dwBuildNumber) + ")";
}

// ── Network helpers ───────────────────────────────────────────────────────────
//
// Both readIpAddress and readMacAddress walk the same adapter list with AF_INET
// so IP and MAC always refer to the same NIC (mirrors the Linux approach).

static PIP_ADAPTER_ADDRESSES findFirstValidAdapter(std::vector<BYTE>& buf) {
  ULONG bufLen = 15'000;
  int retries  = 3;
  ULONG st;

  do {
    buf.resize(bufLen);
    st = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr,
                              reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data()),
                              &bufLen);
  } while (st == ERROR_BUFFER_OVERFLOW && --retries);

  if (st != NO_ERROR) return nullptr;

  auto* adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
  for (; adapter; adapter = adapter->Next) {
    if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
    if (adapter->OperStatus != IfOperStatusUp)         continue;
    if (!adapter->FirstUnicastAddress)                 continue;
    if (adapter->PhysicalAddressLength == 0)           continue;
    return adapter;
  }
  return nullptr;
}

std::string WindowsSystemMonitor::readIpAddress() {
  std::vector<BYTE> buf;
  PIP_ADAPTER_ADDRESSES adapter = findFirstValidAdapter(buf);
  if (!adapter) throw std::runtime_error("No suitable network adapter found");

  for (auto* ua = adapter->FirstUnicastAddress; ua; ua = ua->Next) {
    if (ua->Address.lpSockaddr->sa_family == AF_INET) {
      auto* sa = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
      char ipBuf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &sa->sin_addr, ipBuf, sizeof(ipBuf));
      return std::string(ipBuf);
    }
  }
  throw std::runtime_error("No IPv4 address found on adapter");
}

std::string WindowsSystemMonitor::readMacAddress() {
  std::vector<BYTE> buf;
  PIP_ADAPTER_ADDRESSES adapter = findFirstValidAdapter(buf);
  if (!adapter) throw std::runtime_error("No suitable network adapter found");

  std::ostringstream oss;
  for (DWORD i = 0; i < adapter->PhysicalAddressLength; ++i) {
    if (i > 0) oss << ':';
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(adapter->PhysicalAddress[i]);
  }
  return oss.str();  // "aa:bb:cc:dd:ee:ff"
}

// ── getProcessList ────────────────────────────────────────────────────────────

std::vector<ProcessInfo> WindowsSystemMonitor::getProcessList() {
  std::vector<ProcessInfo> result;

  FILETIME nowFt;
  GetSystemTimeAsFileTime(&nowFt);
  const ULONGLONG now =
      (static_cast<ULONGLONG>(nowFt.dwHighDateTime) << 32) | nowFt.dwLowDateTime;

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return result;

  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);

  // MinGW-w64 only provides W variants in tlhelp32.h; convert WCHAR→UTF-8
  auto wToStr = [](const WCHAR* wide) -> std::string {
    if (!wide || !*wide) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), len, nullptr, nullptr);
    return result;
  };

  for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
    if (pe.th32ProcessID == 0) continue;  // skip the idle pseudo-process

    ProcessInfo info{};
    info.pid  = pe.th32ProcessID;
    info.name = wToStr(pe.szExeFile);

    HANDLE hProc = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, info.pid);

    if (hProc) {
      // ── CPU lifetime average (same formula as Linux) ──────────────────────
      FILETIME creation, exit, kernel, user;
      if (GetProcessTimes(hProc, &creation, &exit, &kernel, &user)) {
        const ULONGLONG created =
            (static_cast<ULONGLONG>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
        const ULONGLONG cpuTicks =
            ((static_cast<ULONGLONG>(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime) +
            ((static_cast<ULONGLONG>(user.dwHighDateTime)   << 32) | user.dwLowDateTime);
        const ULONGLONG elapsed = (now > created) ? (now - created) : 1;
        info.cpu_percent =
            static_cast<float>(cpuTicks) / static_cast<float>(elapsed) * 100.0f;
      }

      // ── Working set in kB (mirrors Linux VmRSS in kB) ────────────────────
      PROCESS_MEMORY_COUNTERS pmc{};
      pmc.cb = sizeof(pmc);
      if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc)))
        info.mem_bytes = pmc.WorkingSetSize / 1024;

      CloseHandle(hProc);
    }

    result.push_back(info);
  }

  CloseHandle(snap);
  return result;
}

// ── executeShell ──────────────────────────────────────────────────────────────

std::string WindowsSystemMonitor::executeShell(const std::string& command) {
  const std::string cmd = "cmd.exe /c " + command;
  FILE* pipe = _popen(cmd.c_str(), "r");
  if (!pipe) return {};

  std::string output;
  char buf[256];
  while (fgets(buf, sizeof(buf), pipe))
    output += buf;

  _pclose(pipe);
  return output;
}

#endif  // _WIN32
