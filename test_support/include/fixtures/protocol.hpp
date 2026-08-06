#ifndef PROTOCOL_FIXTURE_HPP
#define PROTOCOL_FIXTURE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace Protocol {
inline const std::string TEST_IP_STR = "127.1.1.1";

inline const std::uint16_t TEST_IP_LEN =
    static_cast<std::uint16_t>(TEST_IP_STR.size());

inline const std::vector<std::uint8_t> TEST_IP{TEST_IP_STR.begin(),
                                               TEST_IP_STR.end()};

inline const std::string TEST_HOSTNAME_STR = "agent-01";

inline const std::uint16_t TEST_HOSTNAME_LEN =
    static_cast<std::uint16_t>(TEST_HOSTNAME_STR.size());

inline const std::vector<std::uint8_t> TEST_HOSTNAME{TEST_HOSTNAME_STR.begin(),
                                                     TEST_HOSTNAME_STR.end()};

inline const std::string TEST_OS_VERSION_STR = "Ubuntu 22.04";

inline const std::uint16_t TEST_OS_VERSION_LEN =
    static_cast<std::uint16_t>(TEST_OS_VERSION_STR.size());

inline const std::vector<std::uint8_t> TEST_OS_VERSION{
    TEST_OS_VERSION_STR.begin(), TEST_OS_VERSION_STR.end()};

inline const std::string TEST_CURRENT_USER_STR = "test-currentuser";

inline const std::uint16_t TEST_CURRENT_USER_LEN =
    static_cast<std::uint16_t>(TEST_CURRENT_USER_STR.size());

inline const std::vector<std::uint8_t> TEST_CURRENT_USER{
    TEST_CURRENT_USER_STR.begin(), TEST_CURRENT_USER_STR.end()};

inline const std::string TEST_MAC_STR = "AA:BB:CC:DD:EE:FF";

}  // namespace Protocol

#endif