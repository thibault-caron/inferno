#ifndef OS_INFO_BUILDER_HPP
#define OS_INFO_BUILDER_HPP

#include "fixtures/protocol.hpp"
#include "protocol/lptf_protocol.hpp"

namespace OsInfoBuilder {
inline OsInfoPayload create() {
  OsInfoPayload info;
  info.os_type = OSType::LINUX;
  info.arch = ArchType::X64;
  // info.hostname = "agent-01";
  info.hostname = Protocol::TEST_HOSTNAME_STR;
  info.os_version = Protocol::TEST_OS_VERSION_STR;
  // info.os_version = "test-osversion";
  // info.current_user = "test-currentuser";
  info.current_user = Protocol::TEST_CURRENT_USER_STR;
  info.ip = Protocol::TEST_IP_STR;
  info.mac = Protocol::TEST_MAC_STR;
  return info;
}
};  // namespace OsInfoBuilder

#endif