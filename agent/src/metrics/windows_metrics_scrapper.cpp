#ifdef _WIN32

#include "metrics/windows_metrics_scrapper.hpp"

#include <algorithm>
#include <cstring>   // strnlen
#include <stdexcept>

#include "time/time_converter.hpp"
#include "logger.hpp"

// ── Construction / destruction ────────────────────────────────────────────────

WindowsMetricsScrapper::WindowsMetricsScrapper() {
  initPdh();
}

WindowsMetricsScrapper::~WindowsMetricsScrapper() {
  if (query_) PdhCloseQuery(query_);
}

void WindowsMetricsScrapper::initPdh() {
  PDH_STATUS st = PdhOpenQuery(nullptr, 0, &query_);
  if (st != ERROR_SUCCESS)
    throw std::runtime_error("PdhOpenQuery failed: " + std::to_string(st));

  // Wildcard (*) captures _Total + every numbered core (0, 1, 2, …)
  PdhAddEnglishCounterA(query_, "\\Processor(*)\\% Processor Time",     0, &cpuCounter_);
  PdhAddEnglishCounterA(query_, "\\PhysicalDisk(*)\\Disk Read Bytes/sec",  0, &diskReadCounter_);
  PdhAddEnglishCounterA(query_, "\\PhysicalDisk(*)\\Disk Write Bytes/sec", 0, &diskWriteCounter_);

  // First collect establishes the baseline; rate counters need two data points.
  PdhCollectQueryData(query_);
}

// ── PDH wildcard helper ───────────────────────────────────────────────────────

std::vector<WindowsMetricsScrapper::PdhItem>
WindowsMetricsScrapper::getCounterItems(PDH_HCOUNTER counter) {
  DWORD bufSize  = 0;
  DWORD itemCount = 0;

  // First call: size probe — expected to return PDH_MORE_DATA.
  PdhGetFormattedCounterArrayA(counter, PDH_FMT_DOUBLE,
                               &bufSize, &itemCount, nullptr);
  if (bufSize == 0) return {};

  std::vector<BYTE> buf(bufSize);
  auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_A*>(buf.data());

  if (PdhGetFormattedCounterArrayA(counter, PDH_FMT_DOUBLE,
                                   &bufSize, &itemCount, items) != ERROR_SUCCESS)
    return {};

  std::vector<PdhItem> result;
  result.reserve(itemCount);
  for (DWORD i = 0; i < itemCount; ++i) {
    const DWORD status = items[i].FmtValue.CStatus;
    if (status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA)
      result.push_back({items[i].szName, items[i].FmtValue.doubleValue});
  }
  return result;
}

// ── Per-subsystem readers ─────────────────────────────────────────────────────

CpuSample WindowsMetricsScrapper::readCpu() {
  CpuSample cpu{};
  if (firstSample_) return cpu;  // zeros until two PDH data points exist

  auto items = getCounterItems(cpuCounter_);

  // Separate _Total from numbered cores, then sort cores numerically.
  std::vector<PdhItem> cores;
  for (const auto& item : items) {
    if (item.name == "_Total")
      cpu.total_percent = static_cast<float>(item.value);
    else
      cores.push_back(item);
  }

  std::sort(cores.begin(), cores.end(), [](const PdhItem& a, const PdhItem& b) {
    try { return std::stoi(a.name) < std::stoi(b.name); } catch (...) { return false; }
  });
  for (const auto& c : cores)
    cpu.per_core.push_back(static_cast<float>(c.value));

  return cpu;
}

MemSample WindowsMetricsScrapper::readMem() {
  MEMORYSTATUSEX ms{};
  ms.dwLength = sizeof(ms);
  GlobalMemoryStatusEx(&ms);

  MemSample mem{};
  mem.phys_total     = ms.ullTotalPhys;
  mem.phys_available = ms.ullAvailPhys;
  mem.phys_used      = ms.ullTotalPhys - ms.ullAvailPhys;
  mem.swap_total     = ms.ullTotalPageFile;
  mem.swap_used      = ms.ullTotalPageFile - ms.ullAvailPageFile;
  return mem;
}

std::vector<DiskSample> WindowsMetricsScrapper::readDisks() {
  std::vector<DiskSample> result;
  if (firstSample_) return result;  // no previous data point yet

  const auto reads  = getCounterItems(diskReadCounter_);
  const auto writes = getCounterItems(diskWriteCounter_);

  for (const auto& r : reads) {
    if (r.name == "_Total") continue;

    DiskSample disk;
    disk.device             = r.name;
    disk.read_bytes_per_sec = static_cast<float>(r.value);

    auto it = std::find_if(writes.begin(), writes.end(),
                           [&](const PdhItem& w) { return w.name == r.name; });
    disk.write_bytes_per_sec =
        (it != writes.end()) ? static_cast<float>(it->value) : 0.0f;

    result.push_back(disk);
  }
  return result;
}

std::vector<NetSample> WindowsMetricsScrapper::readNet(float elapsed) {
  std::vector<NetSample> result;

  // Size probe
  ULONG size = 0;
  GetIfTable(nullptr, &size, FALSE);
  if (size == 0) return result;

  std::vector<std::uint8_t> buf(size);
  auto* table = reinterpret_cast<PMIB_IFTABLE>(buf.data());

  if (GetIfTable(table, &size, FALSE) != NO_ERROR) return result;

  for (DWORD i = 0; i < table->dwNumEntries; ++i) {
    const MIB_IFROW& row = table->table[i];
    if (row.dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

    NetSample net;

    // bDescr is a fixed-size UCHAR[MAXLEN_IFDESCR]; use strnlen to be safe.
    const char* desc = reinterpret_cast<const char*>(row.bDescr);
    net.iface = std::string(desc, strnlen(desc, row.dwDescrLen));

    auto& prev = previousNet_[row.dwIndex];  // previousNet_ is a proper member

    if (!firstSample_ && elapsed > 0.0f) {
      net.rx_bytes_per_sec =
          static_cast<float>(row.dwInOctets  - prev.rx) / elapsed;
      net.tx_bytes_per_sec =
          static_cast<float>(row.dwOutOctets - prev.tx) / elapsed;
    }

    prev.rx = row.dwInOctets;
    prev.tx = row.dwOutOctets;

    result.push_back(net);
  }
  return result;
}

// ── sample() — single entry point ────────────────────────────────────────────

MetricsSample WindowsMetricsScrapper::sample() {
  MetricsSample s{};
  const auto now = std::chrono::system_clock::now();
  s.timestamp = TimeConverter::systemClockToIso(now);

  float elapsed = 0.0f;
  if (!firstSample_)
    elapsed = std::chrono::duration<float>(now - lastSampleTime_).count();

  // Second PDH collect — rates are now computable for all counters at once.
  PdhCollectQueryData(query_);

  try { s.cpu        = readCpu();        } catch (const std::exception& e) {
    Logger::error("WindowsMetricsScrapper", "readCpu: "   + std::string(e.what())); }
  try { s.mem        = readMem();        } catch (const std::exception& e) {
    Logger::error("WindowsMetricsScrapper", "readMem: "   + std::string(e.what())); }
  try { s.disks      = readDisks();      } catch (const std::exception& e) {
    Logger::error("WindowsMetricsScrapper", "readDisks: " + std::string(e.what())); }
  try { s.interfaces = readNet(elapsed); } catch (const std::exception& e) {
    Logger::error("WindowsMetricsScrapper", "readNet: "   + std::string(e.what())); }

  lastSampleTime_ = now;
  if (firstSample_) firstSample_ = false;
  return s;
}

#endif  // _WIN32