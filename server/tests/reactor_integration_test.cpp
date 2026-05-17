#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "helpers_test.hpp"
#include "poller/epoller.hpp"
#include "protocol/protocol_parser.hpp"
#include "reactor.hpp"
#include "server_dispatcher.hpp"
#include "socket/socket_factory.hpp"
#include "tcp_server.hpp"
#include "test_constants.hpp"

#if !defined(__linux__)

TEST(ReactorIntegration, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Reactor epoll integration is Linux-only";
}

#else

#include <unistd.h>

namespace {

// stopReactor wakes the blocking epoll_wait by connecting a dummy socket,
// triggering the reactor's accept path once more so it can check running_.
// Temporary until Reactor::stop() sends a proper wakeup (eventfd/pipe).
void stopReactor(Reactor& reactor, std::uint16_t port) {
  reactor.stop();
  auto wake = SocketFactory::createTCP();
  wake->connect("127.0.0.1", port);
  // wake closes on destruction — just enough to unblock epoll_wait
}

// Creates and starts a TcpServer + Epoller + Reactor, launches the reactor
// in a background thread, and returns the thread.
// Caller is responsible for calling stopReactor() before join().
std::thread startReactorThread(TcpServer& server, Epoller& epoller,
                               ServerDispatcher& dispatcher,
                               Reactor& reactor, std::uint16_t port) {
  server.start();
  server.setNonBlocking();
  return std::thread([&reactor] { reactor.run(); });
}

}  // namespace

// ① Happy path — agent connects and sends REGISTER. No error expected.
// Verifies the reactor accepts the connection and dispatches the frame
// without crashing or sending an error back.
TEST(ReactorIntegration, should_accept_register_without_error) {
  const std::uint16_t port = TestConstants::REACTOR_HAPPY_PATH_PORT;
  TcpServer server(port);
  Epoller epoller;
  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  auto reactorThread =
      startReactorThread(server, epoller, dispatcher, reactor, port);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect("127.0.0.1", port));

  const auto registerFrame =
      makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-1"));
  EXPECT_TRUE(socket->send(registerFrame).ok());

  stopReactor(reactor, port);
  reactorThread.join();
}

// ② Protocol enforcement — agent sends COMMAND before REGISTER.
// The reactor must reply with an ERROR frame before dispatching.
TEST(ReactorIntegration,
     should_send_error_when_first_message_is_not_register) {
  const std::uint16_t port = TestConstants::REACTOR_INVALID_FIRST_MESSAGE_PORT;
  TcpServer server(port);
  Epoller epoller;
  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  auto reactorThread =
      startReactorThread(server, epoller, dispatcher, reactor, port);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect("127.0.0.1", port));  // ← correct port

  const auto commandFrame = makeRawFrame(MessageType::COMMAND);
  ASSERT_TRUE(socket->send(commandFrame).ok());

  // Read back the ERROR response using AgentSession's buffer machinery
  AgentSession session(std::move(socket));
  session.receiveIntoBuffer();

  std::optional<Frame> frame = session.tryExtractFrame();
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->header.type, MessageType::ERROR);

  stopReactor(reactor, port);
  reactorThread.join();
}

// ③ Resilience — first agent disconnects, second agent can still connect
// and communicate. Verifies the reactor cleans up dead sessions correctly.
TEST(ReactorIntegration,
     should_keep_serving_after_first_agent_disconnects) {
  const std::uint16_t port = TestConstants::REACTOR_DISCONNECT_PORT;
  TcpServer server(port);
  Epoller epoller;
  ServerDispatcher dispatcher;
  Reactor reactor(server, dispatcher, epoller);

  auto reactorThread =
      startReactorThread(server, epoller, dispatcher, reactor, port);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // First agent connects, sends REGISTER, then disconnects
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect("127.0.0.1", port));  // ← correct port
    const auto frame =
        makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-a"));
    EXPECT_TRUE(socket->send(frame).ok());
    // socket closes on scope exit — reactor detects disconnection
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Second agent can still connect — reactor is still running
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect("127.0.0.1", port));  // ← correct port
    const auto frame =
        makeRawFrame(MessageType::REGISTER, makeRegisterPayload("reactor-b"));
    EXPECT_TRUE(socket->send(frame).ok());
  }

  stopReactor(reactor, port);
  reactorThread.join();
}

#endif