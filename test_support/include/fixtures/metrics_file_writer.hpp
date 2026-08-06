#ifndef METRICS_FILE_WRITER_HPP
#define METRICS_FILE_WRITER_HPP

#include <fstream>
#include <string>

namespace MetricsFileWriter {

inline void createFakeProc(const std::string& procRoot) {
  std::ofstream stat(procRoot + "/stat");
  stat << "cpu  1000 0 2000 3000000 500 0 0 0 0 0\n";
  stat << "cpu0 500 0 1000 1500000 250 0 0 0 0 0\n";
  stat << "cpu1 500 0 1000 1500000 250 0 0 0 0 0\n";
  stat.close();

  std::ofstream mem(procRoot + "/meminfo");
  mem << "MemTotal:        8000000 kB\n";
  mem << "MemAvailable:    6000000 kB\n";
  mem << "SwapTotal:       2000000 kB\n";
  mem << "SwapFree:        2000000 kB\n";
  mem.close();

  std::ofstream disk(procRoot + "/diskstats");
  disk << "   8        0 sda 12345 0 98765 0 54321 0 432100 0 0 0 0 0 0 0 "
          "0\n";
  disk.close();

  std::ofstream net(procRoot + "/net/dev");
  net << "Inter-|   Receive                                                "
         "|  Transmit\n";
  net << " face |bytes    packets errs drop fifo frame compressed "
         "multicast|bytes    packets errs drop fifo colls carrier "
         "compressed\n";
  net << "    lo: 1000000     100    0    0    0     0          0         "
         "0  1000000     100    0    0    0     0       0          0\n";
  net << "  eth0: 5000000     500    0    0    0     0          0         "
         "0  2000000     200    0    0    0     0       0          0\n";
  net.close();
}

inline void updateFakeProcFiles(const std::string& procRoot) {
  // Update /proc/stat with different values (simulating time passing)
  std::ofstream out(procRoot + "/stat");
  out << "cpu  2000 0 4000 6000000 1000 0 0 0 0 0\n";
  out << "cpu0 1000 0 2000 3000000 500 0 0 0 0 0\n";
  out << "cpu1 1000 0 2000 3000000 500 0 0 0 0 0\n";
  out.close();

  // Update /proc/meminfo
  std::ofstream mem(procRoot + "/meminfo");
  mem << "MemTotal:        8000000 kB\n";
  mem << "MemAvailable:    5900000 kB\n";
  mem << "SwapTotal:       2000000 kB\n";
  mem << "SwapFree:        1900000 kB\n";
  mem.close();

  // Update /proc/diskstats (more reads/writes)
  std::ofstream disk(procRoot + "/diskstats");
  disk << "   8        0 sda 12500 0 99765 0 55000 0 440100 0 0 0 0 0 0 0 "
          "0\n";
  disk.close();

  // Update /proc/net/dev (more bytes)
  std::ofstream net(procRoot + "/net/dev");
  net << "Inter-|   Receive                                                "
         "|  Transmit\n";
  net << " face |bytes    packets errs drop fifo frame compressed "
         "multicast|bytes    packets errs drop fifo colls carrier "
         "compressed\n";
  net << "    lo: 1100000     110    0    0    0     0          0         "
         "0  1100000     110    0    0    0     0       0          0\n";
  net << "  eth0: 5500000     550    0    0    0     0          0         "
         "0  2500000     250    0    0    0     0       0          0\n";
  net.close();
}

}  // namespace MetricsFileWriter

#endif