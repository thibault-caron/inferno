#include "socket/socket_helper.hpp"

const char* SocketHelper::messageTypeToString(const MessageType type) {
  switch (type) {
    case MessageType::REGISTER:
      return "REGISTER";
    case MessageType::DATA:
      return "DATA";
    case MessageType::COMMAND:
      return "COMMAND";
    case MessageType::RESPONSE:
      return "RESPONSE";
    case MessageType::DISCONNECT:
      return "DISCONNECT";
    case MessageType::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

LptfHeader SocketHelper::createHeader(
    MessageType type, const std::vector<std::uint8_t>& payload) {
  LptfHeader header{{'L', 'P', 'T', 'F'},
                    LPTF_VERSION,
                    type,
                    static_cast<std::uint16_t>(payload.size())};
  return header;
}