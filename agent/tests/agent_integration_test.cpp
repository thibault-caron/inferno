#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "agent_dispatcher.hpp"
#include "agent_session.hpp"
#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/socket_factory.hpp"
#include "socket/socket_helper.hpp"
#include "socket/socket_test_helpers.hpp"

namespace {

constexpr std::uint16_t kPort = 19892;

}  // namespace

TEST(AgentIntegration, should_register_respond_and_disconnect) {
  const int serverFd = startServer(kPort);
  ASSERT_NE(serverFd, -1);

  std::atomic<bool> agentExited{false};
  std::atomic<bool> agentOk{true};

  std::thread agent([&] {
    auto socket = SocketFactory::createTCP();
    if (!socket || !socket->connect("127.0.0.1", kPort)) {
      agentOk = false;
      return;
    }

    AgentSession session(std::move(socket));
    AgentDispatcher dispatcher;
    dispatcher.sendRegister(session);

    while (session.isValid()) {
      const SocketResult result = SocketHelper::receiveIntoBuffer(session);
      if (!result.ok() || result.bytesTransferred <= 0) {
        agentOk = false;
        return;
      }

      while (std::optional<Frame> frame = session.tryExtractFrame()) {
        dispatcher.handleFrame(session, frame.value());
        if (!session.isValid()) {
          agentExited = true;
          return;
        }
      }
    }
  });

  sockaddr_in clientAddress{};
  socklen_t clientLen = sizeof(clientAddress);
  const int clientFd = ::accept(
      serverFd, reinterpret_cast<sockaddr*>(&clientAddress), &clientLen);
  ASSERT_NE(clientFd, -1);

  const std::vector<std::uint8_t> registerHeaderBytes =
      recvExact(clientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(registerHeaderBytes.size(), LPTF_HEADER_SIZE);
  const LptfHeader registerHeader =
      ProtocolParser::parseHeader(registerHeaderBytes);
  EXPECT_EQ(registerHeader.type, MessageType::REGISTER);

  const std::vector<std::uint8_t> registerPayloadBytes =
      recvExact(clientFd, registerHeader.size);
  ASSERT_EQ(registerPayloadBytes.size(), registerHeader.size);
  const RegisterPayload registerPayload =
      ProtocolParser::parseRegisterPayload(registerPayloadBytes);
  EXPECT_EQ(registerPayload.hostname, "inferno-agent");

  CommandPayload command;
  command.id = 0;
  command.type = CommandType::OS_INFO;
  command.data = "";
  const std::vector<std::uint8_t> commandPayload =
      ProtocolSerializer::serializeCommandPayload(command);
  const std::vector<std::uint8_t> commandFrame =
      makeRawFrame(MessageType::COMMAND, commandPayload);
  ASSERT_TRUE(sendAll(clientFd, commandFrame));

  const std::vector<std::uint8_t> responseHeaderBytes =
      recvExact(clientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(responseHeaderBytes.size(), LPTF_HEADER_SIZE);
  const LptfHeader responseHeader =
      ProtocolParser::parseHeader(responseHeaderBytes);
  EXPECT_EQ(responseHeader.type, MessageType::RESPONSE);

  const std::vector<std::uint8_t> responsePayloadBytes =
      recvExact(clientFd, responseHeader.size);
  ASSERT_EQ(responsePayloadBytes.size(), responseHeader.size);
  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(responsePayloadBytes);
  EXPECT_EQ(response.id, 0);
  EXPECT_EQ(response.status, ResponseStatus::OK);
  EXPECT_EQ(response.data, "hello world from agent");

  const std::vector<std::uint8_t> disconnectFrame =
      makeRawFrame(MessageType::DISCONNECT);
  ASSERT_TRUE(sendAll(clientFd, disconnectFrame));

  ::close(clientFd);
  ::close(serverFd);

  agent.join();

  EXPECT_TRUE(agentOk);
  EXPECT_TRUE(agentExited);
}
