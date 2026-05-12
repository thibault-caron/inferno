#include <gtest/gtest.h>

#include <chrono>
#include <memory>
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
// TODO must be deleted when stop will be implemented in reactor
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
  // ASSERT_TRUE(server.start()); // already tested in server tcp or should be
  server.start();
  server.setNonBlocking();
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());

  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::thread reactorThread([&] { reactor.run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // const int clientFd =
  // connectLoopback(TestConstants::REACTOR_HAPPY_PATH_PORT);
  std::unique_ptr<ISocket> clientFd = SocketFactory::createTCP();
  AgentSession session(std::move(clientFd));
  session.connect(TestConstants::SERVER_HOST,
                  TestConstants::REACTOR_HAPPY_PATH_PORT);
  ASSERT_NE(session.getFd(), -1);

  const auto registerFrame =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-1"));
  // ASSERT_TRUE(sendAll(clientFd, registerFrame));
  SocketResult result = session.send(registerFrame);
  ASSERT_TRUE(result.ok());

  // ::close(clientFd);
  session.close();

  stopReactor(reactor, TestConstants::REACTOR_HAPPY_PATH_PORT);
  reactorThread.join();
}

TEST(ReactorIntegration, should_send_error_when_first_message_not_register) {
  TcpServer server(TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT);
  // ASSERT_TRUE(server.start());
  server.start();
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());

  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::thread reactorThread([&] { reactor.run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // const int clientFd =
  //     connectLoopback(TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT);
  std::unique_ptr<ISocket> clientFd = SocketFactory::createTCP();
  AgentSession session(std::move(clientFd));
  session.connect(TestConstants::SERVER_HOST,
                  TestConstants::REACTOR_HAPPY_PATH_PORT);
  ASSERT_NE(session.getFd(), -1);

  const auto commandFrame = makeRawFrame(MessageType::COMMAND);
  SocketResult resultSend = session.send(commandFrame);
  ASSERT_TRUE(resultSend.ok());
  // ASSERT_TRUE(sendAll(clientFd, commandFrame));

  SocketResult resultRecv = session.receiveIntoBuffer();
  // const std::vector<std::uint8_t> errorHeaderBytes =
  //     recvExact(clientFd, LPTF_HEADER_SIZE);
  ASSERT_EQ(session.bufferSize(), LPTF_HEADER_SIZE);
  std::optional<Frame> frame = session.tryExtractFrame();
  Frame actualFrame = frame.value();
  // const LptfHeader errorHeader = ProtocolParser::parseHeader();
  EXPECT_EQ(actualFrame.header.type, MessageType::ERROR);

  // const std::vector<std::uint8_t> errorPayloadBytes =
  //     recvExact(clientFd, errorHeader.size);
  // ASSERT_EQ(errorPayloadBytes.size(), errorHeader.size);

  // ::close(clientFd);
  session.close();

  stopReactor(reactor, TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT);
  reactorThread.join();
}

TEST(ReactorIntegration, should_remove_disconnected_agent_and_keep_serving) {
  TcpServer server(TestConstants::REACTOR_DISCONNECT_PORT);

  server.start();
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());

  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  std::thread reactorThread([&] { reactor.run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // const int firstClientFd =
  //     connectLoopback(TestConstants::REACTOR_DISCONNECT_PORT);
  std::unique_ptr<ISocket> firstClientFd = SocketFactory::createTCP();
  AgentSession firstSession(std::move(firstClientFd));
  firstSession.connect(TestConstants::SERVER_HOST,
                       TestConstants::REACTOR_HAPPY_PATH_PORT);
  ASSERT_NE(firstSession.getFd(), -1);

  const auto firstRegister =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-a"));

  SocketResult firstResult = firstSession.send(firstRegister);
  ASSERT_TRUE(firstResult.ok());

  // ::close(firstClientFd);
  // TODO : why close first agent session here ? Why sleep for 10 milliseconds
  // here ?
  firstSession.close();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // const int secondClientFd =
  //     connectLoopback(TestConstants::REACTOR_DISCONNECT_PORT);
  std::unique_ptr<ISocket> secondClientFd = SocketFactory::createTCP();
  AgentSession secondSession(std::move(secondClientFd));
  secondSession.connect(TestConstants::SERVER_HOST,
                        TestConstants::REACTOR_HAPPY_PATH_PORT);
  ASSERT_NE(secondSession.getFd(), -1);

  const auto secondRegister =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-b"));
  // ASSERT_TRUE(sendAll(secondClientFd, secondRegister));
  SocketResult secondResult = secondSession.send(secondRegister);

  // ::close(secondClientFd);
  secondSession.close();

  stopReactor(reactor, TestConstants::REACTOR_DISCONNECT_PORT);
  reactorThread.join();
}

#endif
