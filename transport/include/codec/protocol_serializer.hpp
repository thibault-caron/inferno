#ifndef LPTF_SERIALIZER_HPP
#define LPTF_SERIALIZER_HPP

// #include <cstddef>
// #include <cstring>

#include <cstdint>
#include <vector>

#include "protocol/lptf_protocol.hpp"

namespace ProtocolSerializer {

std::vector<std::uint8_t> serializeHeader(const LptfHeader& header);
std::vector<std::uint8_t> serializeOsInfoPayload(const OsInfoPayload& payload);
std::vector<std::uint8_t> serializeCommandPayload(
    const CommandPayload& payload);
std::vector<std::uint8_t> serializeResponsePayload(
    const ResponsePayload& payload);
std::vector<std::uint8_t> serializeDataPayload(const DataPayload& payload);
std::vector<std::uint8_t> serializeErrorPayload(const ErrorPayload& payload);

std::vector<std::uint8_t> serializeProcessInfo(const ProcessInfo& info);

std::vector<std::uint8_t> serializeProcessInfoList(
    const std::vector<ProcessInfo>& infos);

std::vector<std::uint8_t> serializeDashboardCommand(
    const DashboardCommand& command);
std::vector<std::uint8_t> serializeDashboardData(const DashboardData& data);
std::vector<std::uint8_t> serializeDashboardResponse(
    const DashboardResponse& response);
std::vector<std::uint8_t> serializeRegisterPayload(
    const RegisterPayload& payload);

std::vector<std::uint8_t> serializeRegisterPayloadList(
    const std::vector<RegisterPayload>& registrations);

std::vector<std::uint8_t> serializeDashboardDisconnect(
    const DashboardDisconnect& payload);

std::vector<std::uint8_t> serializeFrame(const Frame& frame);

inline std::vector<std::uint8_t> toBytes(const std::string& s) {
  return std::vector<std::uint8_t>(s.begin(), s.end());
}
};  // namespace ProtocolSerializer

#endif