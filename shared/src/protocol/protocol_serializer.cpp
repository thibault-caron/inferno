#include "protocol/protocol_serializer.hpp"

std::vector<uint8_t> ProtocolSerializer::serializeHeader(LptfHeader header) {
  std::vector<uint8_t> headerInByte;
  headerInByte.resize(8);

  for (size_t i = 0; i < sizeof(header.identifier); ++i) {
    headerInByte[i] = header.identifier[i];
  }

  headerInByte[4] = header.version;
  headerInByte[5] = static_cast<uint8_t>(header.type);

  ConvertEndian::writeU16BE(headerInByte, 6, header.size);

  return headerInByte;
}