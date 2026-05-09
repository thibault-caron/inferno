#ifndef PROTOCOL_HELPER_HPP
#define PROTOCOL_HELPER_HPP

// #include <iostream>

#include <cstddef>
#include <cstdint>
#include <vector>
#include "protocol/lptf_protocol.hpp"

// #include "agent_session.hpp"
// #include "exception/lptf_exception.hpp"
// #include "exception/socket_exception.hpp"
// #include "protocol/protocol_serializer.hpp"
// #include "socket/i_socket.hpp"
// #include "socket/socket_factory.hpp"
// #include "tcp_server.hpp"

class ProtocolHelper {
 public:
  ProtocolHelper() = delete;
  ProtocolHelper(const ProtocolHelper&) = delete;
  ProtocolHelper& operator=(const ProtocolHelper&) = delete;

  // static SocketResult receiveIntoBuffer(AgentSession& session);

  static const char* messageTypeToString(const MessageType type);

  static LptfHeader createHeader(MessageType type,
                                 const std::vector<std::uint8_t>& payload);

  static const std::size_t kReceiveChunkSize = 4096;

//  private:
};

#endif