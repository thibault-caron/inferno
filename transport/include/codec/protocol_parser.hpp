#ifndef LPTF_PARSER_HPP
#define LPTF_PARSER_HPP

// #include <cstring>

#include <cstdint>
#include <vector>

// #include "codec/convert_endian.hpp"
// #include "exception/lptf_exception.hpp"
#include "protocol/lptf_protocol.hpp"

namespace ProtocolParser {

LptfHeader parseHeader(const std::vector<std::uint8_t>& input);
OsInfoPayload parseOsInfoPayload(const std::vector<std::uint8_t>& input,
                                 std::size_t& offset);
OsInfoPayload parseOsInfoPayload(const std::vector<std::uint8_t>& input);

CommandPayload parseCommandPayload(const std::vector<std::uint8_t>& input);
ResponsePayload parseResponsePayload(const std::vector<std::uint8_t>& input);
DataPayload parseDataPayload(const std::vector<std::uint8_t>& input);
ErrorPayload parseErrorPayload(const std::vector<std::uint8_t>& input);
ProcessInfo parseProcessInfo(const std::vector<std::uint8_t>& input);
std::vector<ProcessInfo> parseProcessInfoList(
    const std::vector<std::uint8_t>& input);

DashboardCommand parseDashboardCommand(const std::vector<std::uint8_t>& input);
DashboardData parseDashboardData(const std::vector<std::uint8_t>& input);
DashboardResponse parseDashboardResponse(
    const std::vector<std::uint8_t>& input);
RegisterPayload parseRegisterPayload(const std::vector<std::uint8_t>& input,
                                     std::size_t& offset);

RegisterPayload parseRegisterPayload(const std::vector<std::uint8_t>& input);

std::vector<RegisterPayload> parseRegisterPayloadList(
    const std::vector<std::uint8_t>& input);

DashboardDisconnect parseDashboardDisconnect(
    const std::vector<std::uint8_t>& input);

inline std::string toString(const std::vector<std::uint8_t>& v) {
  return std::string(v.begin(), v.end());
}
};  // namespace ProtocolParser

#endif