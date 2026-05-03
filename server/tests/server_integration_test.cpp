#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <optional>
#include <thread>
#include <vector>

#include "agent_session.hpp"
#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
#include "server_dispatcher.hpp"
#include "socket/ISocket.hpp"
#include "socket/socket_test_helpers.hpp"
#include "tcp_server.hpp"

namespace {

constexpr std::uint16_t kPort = 19882;
constexpr std::size_t kChunkSize = 4096;

SocketResult receiveIntoSession(AgentSession& session) {
  std::vector<std::uint8_t> temp(kChunkSize);
  const SocketResult result = session.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    session.buffer.insert(session.buffer.end(), temp.begin(), temp.end());
  }

  return result;
}

std::optional<Frame> receiveOneFrame(AgentSession& session) {
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

TEST(ServerIntegration, should_handle_register_response_and_disconnect) {
  TcpServer server(kPort);
  ASSERT_TRUE(server.start());

  bool agentSawCommand = false;
  bool agentSawDisconnect = false;
  std::uint16_t agentObservedCommandId = 0;
  CommandType agentObservedCommandType = CommandType::END;

  std::thread agentThread([&] {
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
      agentSawCommand = commandHeader.type == MessageType::COMMAND;

      if (agentSawCommand) {
        const std::vector<std::uint8_t> commandPayloadBytes =
            recvExact(fd, commandHeader.size);
        if (commandPayloadBytes.size() == commandHeader.size) {
          const CommandPayload command =
              ProtocolParser::parseCommandPayload(commandPayloadBytes);
          agentObservedCommandId = command.id;
          agentObservedCommandType = command.type;

          const auto responseFrame = makeRawFrame(
              MessageType::RESPONSE, makeResponsePayload(command.id, "done"));
          if (sendAll(fd, responseFrame)) {
            const std::vector<std::uint8_t> disconnectHeaderBytes =
                recvExact(fd, LPTF_HEADER_SIZE);
            if (disconnectHeaderBytes.size() == LPTF_HEADER_SIZE) {
              const LptfHeader disconnectHeader =
                  ProtocolParser::parseHeader(disconnectHeaderBytes);
              agentSawDisconnect =
                  disconnectHeader.type == MessageType::DISCONNECT &&
                  disconnectHeader.size == 0;
            }
          }
        }
      }
    }

    ::close(fd);
  });

  std::unique_ptr<ISocket> accepted = server.acceptAgent();
  ASSERT_NE(accepted, nullptr);

  AgentSession session(std::move(accepted));
  ServerDispatcher dispatcher;

  const std::optional<Frame> firstFrame = receiveOneFrame(session);
  ASSERT_TRUE(firstFrame.has_value());
  EXPECT_EQ(firstFrame->header.type, MessageType::REGISTER);
  EXPECT_FALSE(session.isRegistered());

  dispatcher.handleFrame(session, firstFrame.value());

  EXPECT_TRUE(session.isRegistered());
  EXPECT_EQ(session.getAgentInfo().hostname, "worker-42");

  const std::optional<Frame> secondFrame = receiveOneFrame(session);
  ASSERT_TRUE(secondFrame.has_value());
  EXPECT_EQ(secondFrame->header.type, MessageType::RESPONSE);

  dispatcher.handleFrame(session, secondFrame.value());

  agentThread.join();

  EXPECT_TRUE(agentSawCommand);
  EXPECT_EQ(agentObservedCommandId, 0);
  EXPECT_EQ(agentObservedCommandType, CommandType::OS_INFO);
  EXPECT_TRUE(agentSawDisconnect);
}