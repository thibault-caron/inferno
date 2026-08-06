#ifndef PROCESS_BUILDER_HPP
#define PROCESS_BUILDER_HPP

#include "protocol/lptf_protocol.hpp"

namespace ProcessBuilder {
inline ProcessInfo createProcessInfo(std::uint32_t pid = 1234,
                                     float cpu_percent = 42.5f,
                                     std::uint64_t mem_bytes = 1024 * 1024 *
                                                               512,
                                     std::string name = "my_process") {
  ProcessInfo info;
  info.pid = pid;
  info.cpu_percent = cpu_percent;
  info.mem_bytes = mem_bytes;  // 512 MB
  info.name = name;
  return info;
}

inline std::vector<ProcessInfo> createProcessInfoList(
    std::uint32_t howMayProcess = 3) {
  std::vector<ProcessInfo> infos;
  std::uint32_t pid = 1234;
  for (std::uint32_t i{0}; i < howMayProcess; ++i) {
    std::uint32_t currentPid = pid;
    infos.insert(infos.end(), ProcessBuilder::createProcessInfo(currentPid += i));
  }
  return infos;
}

}  // namespace ProcessBuilder
#endif