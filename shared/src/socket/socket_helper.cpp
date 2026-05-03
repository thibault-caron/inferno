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

SocketResult SocketHelper::receiveIntoBuffer(AgentSession& session) {
  std::vector<std::uint8_t> temp(kReceiveChunkSize);
  const SocketResult result = session.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    session.buffer.insert(session.buffer.end(), temp.begin(), temp.end());
    std::cout << "[agent] recv bytes=" << result.bytesTransferred
              << " buffer_size=" << session.buffer.size() << "\n";
  } else {
    std::cout << "[agent] recv status=" << static_cast<int>(result.error)
              << " bytes=" << result.bytesTransferred << "\n";
  }
  return result;
};

LptfHeader SocketHelper::createHeader(
    MessageType type, const std::vector<std::uint8_t>& payload) {
  LptfHeader header{{'L', 'P', 'T', 'F'},
                    LPTF_VERSION,
                    type,
                    static_cast<std::uint16_t>(payload.size())};
  return header;
}