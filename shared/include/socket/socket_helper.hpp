#ifndef SOCKET_HELPER
#define SOCKET_HELPER

#include <iostream>

#include "agent_session.hpp"
#include "exception/lptf_exception.hpp"
#include "exception/socket_exception.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/ISocket.hpp"
#include "socket/socket_factory.hpp"
// #include "tcp_server.hpp"

class SocketHelper {
 public:
  SocketHelper() = delete;
  SocketHelper(const SocketHelper&) = delete;
  SocketHelper& operator=(const SocketHelper&) = delete;

  static SocketResult receiveIntoBuffer(AgentSession& session);

  static const char* messageTypeToString(const MessageType type);

  static LptfHeader createHeader(
      MessageType type, const std::vector<std::uint8_t>& payload);

 private:
  static const std::size_t kReceiveChunkSize = 4096;
};

#endif