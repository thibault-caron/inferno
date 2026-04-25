#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <optional>
#include <thread>
#include <vector>

#include "client_session.hpp"
#include "dispatcher.hpp"
#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
#include "socket/ISocket.hpp"
#include "tcp_server.hpp"

namespace {

constexpr std::uint16_t kPort = 19882;
constexpr std::size_t kChunkSize = 4096;

int connectLoopback(std::uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) return -1;

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
    ::close(fd);
    return -1;
  }

  if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
      0) {
    ::close(fd);
    return -1;
  }

  return fd;
}

bool sendAll(int fd, const std::vector<std::uint8_t>& bytes) {
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const ssize_t sent =
        ::send(fd, bytes.data() + offset, bytes.size() - offset, 0);
    if (sent <= 0) return false;
    offset += static_cast<std::size_t>(sent);
  }
  return true;
}

std::vector<std::uint8_t> recvExact(int fd, std::size_t size) {
  std::vector<std::uint8_t> bytes(size);
  std::size_t offset = 0;
  while (offset < size) {
    const ssize_t received =
        ::recv(fd, bytes.data() + offset, size - offset, 0);
    if (received <= 0) {
      bytes.resize(offset);
      return bytes;
    }
    offset += static_cast<std::size_t>(received);
  }
  return bytes;
}

SocketResult receiveIntoSession(ClientSession& session) {
  std::vector<std::uint8_t> temp(kChunkSize);
  const SocketResult result = session.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    session.buffer.insert(session.buffer.end(), temp.begin(), temp.end());
  }

  return result;
}

std::optional<Frame> receiveOneFrame(ClientSession& session) {
  while (true) {
    if (std::optional<Frame> frame = session.tryExtractFrame()) {
      return frame;
    }

    const SocketResult result = receiveIntoSession(session);
    if (!result.ok() || result.bytesTransferred <= 0) {
      return std::nullopt;
    }
  }
}

}  // namespace

TEST(ServerIntegration,
     should_register_client_before_accepting_other_message_types) {
  TcpServer server(kPort);
  ASSERT_TRUE(server.start());

  bool clientSawCommand = false;
  bool clientSawDisconnect = false;
  std::uint16_t clientObservedCommandId = 0;
  CommandType clientObservedCommandType = CommandType::END;

  std::thread clientThread([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const int fd = connectLoopback(kPort);
    if (fd == -1) return;

    const auto registerFrame =
        makeRawFrame(MessageType::REGISTER, makeRegisterPayload("worker-42"));
    if (!sendAll(fd, registerFrame)) {
      ::close(fd);
      return;
    }

    const std::vector<std::uint8_t> commandHeaderBytes =
        recvExact(fd, LPTF_HEADER_SIZE);
    if (commandHeaderBytes.size() == LPTF_HEADER_SIZE) {
      const LptfHeader commandHeader =
          ProtocolParser::parseHeader(commandHeaderBytes);
      clientSawCommand = commandHeader.type == MessageType::COMMAND;

      if (clientSawCommand) {
        const std::vector<std::uint8_t> commandPayloadBytes =
            recvExact(fd, commandHeader.size);
        if (commandPayloadBytes.size() == commandHeader.size) {
          const CommandPayload command =
              ProtocolParser::parseCommandPayload(commandPayloadBytes);
          clientObservedCommandId = command.id;
          clientObservedCommandType = command.type;

          const auto responseFrame = makeRawFrame(
              MessageType::RESPONSE, makeResponsePayload(command.id, "done"));
          if (sendAll(fd, responseFrame)) {
            const std::vector<std::uint8_t> disconnectHeaderBytes =
                recvExact(fd, LPTF_HEADER_SIZE);
            if (disconnectHeaderBytes.size() == LPTF_HEADER_SIZE) {
              const LptfHeader disconnectHeader =
                  ProtocolParser::parseHeader(disconnectHeaderBytes);
              clientSawDisconnect =
                  disconnectHeader.type == MessageType::DISCONNECT &&
                  disconnectHeader.size == 0;
            }
          }
        }
      }
    }

    ::close(fd);
  });

  std::unique_ptr<ISocket> accepted = server.acceptClient();
  ASSERT_NE(accepted, nullptr);

  ClientSession session(std::move(accepted));
  Dispatcher dispatcher;

  const std::optional<Frame> firstFrame = receiveOneFrame(session);
  ASSERT_TRUE(firstFrame.has_value());
  EXPECT_EQ(firstFrame->header.type, MessageType::REGISTER);
  EXPECT_FALSE(session.isRegistered());

  dispatcher.dispatch(session, firstFrame.value());

  EXPECT_TRUE(session.isRegistered());
  std::string hostname = session.getClientInfo().hostname;
  EXPECT_EQ(hostname, "worker-42");

  const std::optional<Frame> secondFrame = receiveOneFrame(session);
  ASSERT_TRUE(secondFrame.has_value());
  EXPECT_EQ(secondFrame->header.type, MessageType::RESPONSE);

  dispatcher.dispatch(session, secondFrame.value());

  clientThread.join();

  EXPECT_TRUE(clientSawCommand);
  EXPECT_EQ(clientObservedCommandId, 0);
  EXPECT_EQ(clientObservedCommandType, CommandType::OS_INFO);
  EXPECT_TRUE(clientSawDisconnect);
}