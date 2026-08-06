#include <cstddef>
#include <iostream>
#include <string>

#include "codec/convert_endian.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "exception/lptf_exception.hpp"

namespace ProtocolSerializer {

std::vector<std::uint8_t> serializeOsInfoPayload(const OsInfoPayload& payload) {
  ProtocolHelper::validateOsInfoPayload(payload);

  const std::size_t finalSize{OS_INFO_FIXED_BYTES + payload.hostname.size() +
                              payload.os_version.size() +
                              payload.current_user.size() + payload.ip.size() +
                              payload.mac.size()};
  std::vector<std::uint8_t> payloadInByte(finalSize);

  payloadInByte[0] = static_cast<std::uint8_t>(payload.os_type);
  payloadInByte[1] = static_cast<std::uint8_t>(payload.arch);
  std::size_t offset{2};
  ConvertEndian::writeU16BE(
      payloadInByte, offset,
      static_cast<std::uint16_t>(payload.hostname.size()));

  ConvertEndian::writeU16BE(
      payloadInByte, offset,
      static_cast<std::uint16_t>(payload.os_version.size()));

  ConvertEndian::writeU16BE(
      payloadInByte, offset,
      static_cast<std::uint16_t>(payload.current_user.size()));

  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.ip.size()));

  ConvertEndian::writeU16BE(payloadInByte, offset,
                            static_cast<std::uint16_t>(payload.mac.size()));

  ConvertEndian::writeString(payloadInByte, offset, payload.hostname);

  // ProtocolHelper::copyString(payloadInByte, OS_INFO_FIXED_BYTES,
  //                            payload.hostname);

  // ProtocolHelper::copyString(payloadInByte,
  //                            OS_INFO_FIXED_BYTES + payload.hostname.size(),
  //                            payload.os_version);
  ConvertEndian::writeString(payloadInByte, offset, payload.os_version);
  // ProtocolHelper::copyString(
  //     payloadInByte,
  //     OS_INFO_FIXED_BYTES + payload.hostname.size() +
  //     payload.os_version.size(), payload.current_user);
  ConvertEndian::writeString(payloadInByte, offset, payload.current_user);

  // ProtocolHelper::copyString(payloadInByte,
  //                            OS_INFO_FIXED_BYTES + payload.hostname.size() +
  //                                payload.os_version.size() +
  //                                payload.current_user.size(),
  //                            payload.ip);
  ConvertEndian::writeString(payloadInByte, offset, payload.ip);

  // ProtocolHelper::copyString(payloadInByte,
  //                            OS_INFO_FIXED_BYTES + payload.hostname.size() +
  //                                payload.os_version.size() +
  //                                payload.current_user.size() +
  //                                payload.ip.size(),
  //                            payload.mac);
  ConvertEndian::writeString(payloadInByte, offset, payload.mac);
  return payloadInByte;
}

std::vector<std::uint8_t> serializeProcessInfo(const ProcessInfo& info) {
  ProtocolHelper::validateProcessInfo(info);
  std::vector<uint8_t> payload(PROCESS_INFO_FIXED_SIZE + info.name.size());
  std::size_t offset{0};
  ConvertEndian::writeU32BE(payload, offset, info.pid);
  ConvertEndian::writeFloat(payload, offset, info.cpu_percent);
  ConvertEndian::writeU64BE(payload, offset, info.mem_bytes);
  ConvertEndian::writeU16BE(payload, offset, info.name.size());
  // std::copy(info.name.begin(), info.name.end(), payload.begin() + offset);
  ConvertEndian::writeString(payload, offset, info.name);

  return payload;
}

std::vector<std::uint8_t> serializeProcessInfoList(
    const std::vector<ProcessInfo>& infos) {
  std::size_t totalSize = sizeof(std::uint16_t);  // processCount

  for (const ProcessInfo& info : infos) {
    totalSize += (PROCESS_INFO_FIXED_SIZE + info.name.size());
    // totalSize += info.name.size();
  }

  std::vector<std::uint8_t> finalList(totalSize);
  std::size_t offset{0};
  std::uint16_t processCount = static_cast<std::uint16_t>(infos.size());

  ConvertEndian::writeU16BE(finalList, offset, processCount);

  for (const ProcessInfo& info : infos) {
    std::vector<uint8_t> infoPayload = serializeProcessInfo(info);
    ConvertEndian::writeByteVector(finalList, offset, infoPayload);
    // std::copy(infoPayload.begin(), infoPayload.end(),
    //           finalList.begin() + offset);
    // offset += infoPayload.size();
  }

  return finalList;
}

std::vector<std::uint8_t> serializeRegisterPayload(
    const RegisterPayload& payload) {
  std::uint8_t isOnline = static_cast<std::uint8_t>(payload.online);
  std::uint16_t idLen = payload.id.size();
  std::uint16_t registeredAtLen = payload.registered_at.size();
  std::uint16_t lastSeenLen = payload.last_seen.size();

  std::vector<uint8_t> registerPayload = serializeOsInfoPayload(payload.system);

  std::size_t totalSize{3 * sizeof(std::uint16_t) + idLen + registeredAtLen +
                        lastSeenLen + sizeof(std::uint8_t) +
                        registerPayload.size()};

  std::vector<std::uint8_t> finalPayload(totalSize);
  std::size_t offset{0};

  std::cout << "in serializeRegisterPayload before .at(offset) for is online \n";
  finalPayload.at(offset) = isOnline;
  std::cout << "in serializeRegisterPayload after .at(offset) for is online \n";
  offset += sizeof(std::uint8_t);

  ConvertEndian::writeU16BE(finalPayload, offset, idLen);
  ConvertEndian::writeU16BE(finalPayload, offset, registeredAtLen);
  ConvertEndian::writeU16BE(finalPayload, offset, lastSeenLen);

  ConvertEndian::writeString(finalPayload, offset, payload.id);
  // std::copy(payload.id.begin(), payload.id.end(),
  //           finalPayload.begin() + offset);
  // offset += idLen;
  ConvertEndian::writeString(finalPayload, offset, payload.registered_at);
  // std::copy(payload.registered_at.begin(), payload.registered_at.end(),
  //           finalPayload.begin() + offset);
  // offset += registeredAtLen;
  ConvertEndian::writeString(finalPayload, offset, payload.last_seen);
  // std::copy(payload.last_seen.begin(), payload.last_seen.end(),
  //           finalPayload.begin() + offset);
  // offset += lastSeenLen;

  ConvertEndian::writeByteVector(finalPayload, offset, registerPayload);
  // std::copy(registerPayload.begin(), registerPayload.end(),
  //           finalPayload.begin() + offset);

  return finalPayload;
}

std::vector<std::uint8_t> serializeRegisterPayloadList(
    const std::vector<RegisterPayload>& registrations) {
  std::size_t totalSize = sizeof(std::uint16_t);  // registerCount

  for (const RegisterPayload& payload : registrations) {
    std::size_t registerPayloadSize =
        (REGISTER_FIXED_SIZE +
         // 3 * sizeof(std::uint16_t) +  // id, registered_at, last_seen lengths
         payload.id.size() + payload.registered_at.size() +
         payload.last_seen.size() +
         OS_INFO_FIXED_BYTES +  // OsInfo fixed fields
         payload.system.hostname.size() + payload.system.os_version.size() +
         payload.system.current_user.size() + payload.system.ip.size() +
         payload.system.mac.size());
    totalSize += registerPayloadSize;
  }

  std::vector<std::uint8_t> finalList(totalSize);
  std::size_t offset{0};
  std::uint16_t registrationCount =
      static_cast<std::uint16_t>(registrations.size());

  ConvertEndian::writeU16BE(finalList, offset, registrationCount);

  for (const RegisterPayload& payload : registrations) {
    std::vector<uint8_t> registration = serializeRegisterPayload(payload);
    std::copy(registration.begin(), registration.end(),
              finalList.begin() + offset);
    offset += registration.size();
  }

  return finalList;
}

}  // namespace ProtocolSerializer