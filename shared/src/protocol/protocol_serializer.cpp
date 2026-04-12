#include "protocol/protocol_serializer.hpp"

std::vector<uint8_t> ProtocolSerializer::serializeHeader(const LptfHeader& header) {
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

std::vector<uint8_t> ProtocolSerializer::serializeErrorPayload(const ErrorPayload& payload) {
  std::vector<uint8_t> payloadInByte;

  size_t offsetForMessage{sizeof(uint8_t) + sizeof(uint16_t)};
  size_t message_length = payload.message.size();
  size_t finalSize{offsetForMessage + message_length};

  payloadInByte.resize(finalSize);
  
  payloadInByte[0] = static_cast<uint8_t>(payload.code);
  ConvertEndian::writeU16BE(payloadInByte, 1, message_length);
  
  for (size_t i = 0; i < message_length; ++i ) {
    payloadInByte[i + offsetForMessage] = payload.message[i];
  }
  return payloadInByte;
}