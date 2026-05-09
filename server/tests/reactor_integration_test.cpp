#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "helpers_test.hpp"
#include "poller/epoller.hpp"
#include "protocol/protocol_parser.hpp"
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

TEST(ReactorIntegration, should_accept_register_without_error) {
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

    const auto commandFrame = makeRawFrame(MessageType::COMMAND);
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

  ::close(firstClientFd);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const int secondClientFd =
      connectLoopback(TestConstants::REACTOR_DISCONNECT_PORT);
  ASSERT_NE(secondClientFd, -1);

  const auto secondRegister =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-b"));
  ASSERT_TRUE(sendAll(secondClientFd, secondRegister));

  ::close(secondClientFd);

  stopReactor(reactor, TestConstants::REACTOR_DISCONNECT_PORT);
  reactorThread.join();
}

#endif
