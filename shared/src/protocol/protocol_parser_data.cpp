#include "protocol/protocol_parser.hpp"

namespace {

DataType toDataType(std::uint8_t value) {
  if (value >= static_cast<std::uint8_t>(DataType::END)) {
    throw InvalidFieldValue("data_type",
                            std::to_string(static_cast<unsigned int>(value)));
  }
  return static_cast<DataType>(value);
}

}  // namespace

DataPayload ProtocolParser::parseDataPayload(
    const std::vector<std::uint8_t>& input) {
  if (input.size() < DATA_FIXED_BYTES) {
    throw InvalidSize("data payload", std::to_string(input.size()));
  }
  const std::uint16_t dataLen{ConvertEndian::readU16BE(input, 1)};
  const std::size_t expectedSize{DATA_FIXED_BYTES + dataLen};
  const std::size_t maxLength{MAX_VALUE_INT16 - DATA_FIXED_BYTES};

  validateStringLength(dataLen, input, maxLength, expectedSize);

  DataPayload payload;
  payload.subtype = toDataType(input[0]);
  payload.data.assign(
      reinterpret_cast<const char*>(input.data() + DATA_FIXED_BYTES), dataLen);
  return payload;
}
