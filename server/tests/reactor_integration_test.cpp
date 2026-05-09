#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "helpers_test.hpp"
#include "poller/epoller.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "reactor.hpp"
#include "server_dispatcher.hpp"
#include "socket/socket_test_helpers.hpp"
#include "tcp_server.hpp"
#include "test_constants.hpp"

#if !defined(__linux__)

TEST(ReactorIntegration, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Reactor epoll integration is Linux-only";
}

#else

#include <unistd.h>

namespace {

void stopReactor(Reactor& reactor, std::uint16_t port) {
  reactor.stop();
  const int fd = connectLoopback(port);
  if (fd != -1) {
    ::close(fd);
  }
}

}  // namespace

TEST(ReactorIntegration, should_accept_register_and_send_command) {
  TcpServer server(TestConstants::REACTOR_HAPPY_PATH_PORT);
  ASSERT_TRUE(server.start());

  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::thread reactorThread([&] { reactor.run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const int clientFd = connectLoopback(TestConstants::REACTOR_HAPPY_PATH_PORT);
  ASSERT_NE(clientFd, -1);

  const auto registerFrame =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-1"));
  ASSERT_TRUE(sendAll(clientFd, registerFrame));

  const std::vector<std::uint8_t> commandHeaderBytes =
      recvExact(clientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(commandHeaderBytes.size(), LPTF_HEADER_SIZE);
  const LptfHeader commandHeader =
      ProtocolParser::parseHeader(commandHeaderBytes);
  EXPECT_EQ(commandHeader.type, MessageType::COMMAND);

  const std::vector<std::uint8_t> commandPayloadBytes =
      recvExact(clientFd, commandHeader.size);
  ASSERT_EQ(commandPayloadBytes.size(), commandHeader.size);

  ::close(clientFd);

  stopReactor(reactor, TestConstants::REACTOR_HAPPY_PATH_PORT);
  reactorThread.join();
}

TEST(ReactorIntegration, should_send_error_when_first_message_not_register) {
  TcpServer server(TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT);
  ASSERT_TRUE(server.start());

  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::thread reactorThread([&] { reactor.run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const int clientFd =
      connectLoopback(TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT);
  ASSERT_NE(clientFd, -1);

  CommandPayload command;
  command.id = 0;
  command.type = CommandType::OS_INFO;
  command.data = "";
  const std::vector<std::uint8_t> commandPayload =
      ProtocolSerializer::serializeCommandPayload(command);
  const auto commandFrame = makeRawFrame(MessageType::COMMAND, commandPayload);
  ASSERT_TRUE(sendAll(clientFd, commandFrame));

  const std::vector<std::uint8_t> errorHeaderBytes =
      recvExact(clientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(errorHeaderBytes.size(), LPTF_HEADER_SIZE);
  const LptfHeader errorHeader = ProtocolParser::parseHeader(errorHeaderBytes);
  EXPECT_EQ(errorHeader.type, MessageType::ERROR);

  const std::vector<std::uint8_t> errorPayloadBytes =
      recvExact(clientFd, errorHeader.size);
  ASSERT_EQ(errorPayloadBytes.size(), errorHeader.size);

  ::close(clientFd);

  stopReactor(reactor, TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT);
  reactorThread.join();
}

TEST(ReactorIntegration, should_remove_disconnected_agent_and_keep_serving) {
  TcpServer server(TestConstants::REACTOR_DISCONNECT_PORT);
  ASSERT_TRUE(server.start());

  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::thread reactorThread([&] { reactor.run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const int firstClientFd =
      connectLoopback(TestConstants::REACTOR_DISCONNECT_PORT);
  ASSERT_NE(firstClientFd, -1);

  const auto firstRegister =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-a"));
  ASSERT_TRUE(sendAll(firstClientFd, firstRegister));

  const std::vector<std::uint8_t> commandHeaderBytes =
      recvExact(firstClientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(commandHeaderBytes.size(), LPTF_HEADER_SIZE);
  const LptfHeader firstCommandHeader =
      ProtocolParser::parseHeader(commandHeaderBytes);
  EXPECT_EQ(firstCommandHeader.type, MessageType::COMMAND);
  const std::vector<std::uint8_t> firstCommandPayloadBytes =
      recvExact(firstClientFd, firstCommandHeader.size);
  ASSERT_EQ(firstCommandPayloadBytes.size(), firstCommandHeader.size);
  ::close(firstClientFd);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const int secondClientFd =
      connectLoopback(TestConstants::REACTOR_DISCONNECT_PORT);
  ASSERT_NE(secondClientFd, -1);

  const auto secondRegister =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-b"));
  ASSERT_TRUE(sendAll(secondClientFd, secondRegister));

  const std::vector<std::uint8_t> secondCommandHeaderBytes =
      recvExact(secondClientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(secondCommandHeaderBytes.size(), LPTF_HEADER_SIZE);
  const LptfHeader secondCommandHeader =
      ProtocolParser::parseHeader(secondCommandHeaderBytes);
  EXPECT_EQ(secondCommandHeader.type, MessageType::COMMAND);
  const std::vector<std::uint8_t> secondCommandPayloadBytes =
      recvExact(secondClientFd, secondCommandHeader.size);
  ASSERT_EQ(secondCommandPayloadBytes.size(), secondCommandHeader.size);

  ::close(secondClientFd);

  stopReactor(reactor, TestConstants::REACTOR_DISCONNECT_PORT);
  reactorThread.join();
}

#endif
