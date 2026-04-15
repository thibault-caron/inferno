#include "protocol/protocol_parser.hpp"

namespace {

ResponseStatus toResponseStatus(uint8_t value) {
  if (value <= static_cast<uint8_t>(ResponseStatus::ERROR)) {
    return static_cast<ResponseStatus>(value);
  }
  throw InvalidFieldValue("response_status",
                          std::to_string(static_cast<unsigned int>(value)));
}

void validateChunkFields(uint8_t totalChunks, uint8_t chunkIndex) {
  if (totalChunks == 0) {
    throw InvalidFieldValue("total_chunks", "0");
  }
  if (chunkIndex >= totalChunks) {
    throw InvalidFieldValue(
        "chunk_index", std::to_string(static_cast<unsigned int>(chunkIndex)));
  }
}

}  // namespace

ResponsePayload ProtocolParser::parseResponsePayload(
    const std::vector<uint8_t>& input) {
  if (input.size() < RESPONSE_FIXED_BYTES) {
    throw InvalidSize("response payload", std::to_string(input.size()));
  }
  validateChunkFields(input[3], input[4]);
  const uint16_t dataLen = ConvertEndian::readU16BE(input, 5);
  const std::size_t expectedSize = RESPONSE_FIXED_BYTES + dataLen;
  if (input.size() != expectedSize) {
    throw InvalidSize("response payload", std::to_string(input.size()));
  }

  ResponsePayload payload;
  payload.id = ConvertEndian::readU16BE(input, 0);
  payload.status = toResponseStatus(input[2]);
  payload.total_chunks = input[3];
  payload.chunk_index = input[4];
  payload.data.assign(
      reinterpret_cast<const char*>(input.data() + RESPONSE_FIXED_BYTES),
      dataLen);
  return payload;
}
