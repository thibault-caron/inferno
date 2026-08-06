#include "poller/epoller.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <future>
#include <thread>
#include <vector>

#if !defined(__linux__)

TEST(Epoller, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Epoller is Linux-only";
}

#else

#include "fixtures/ports.hpp"
#include "network/socket_test_utils.hpp"
#include "socket/i_socket.hpp"
#include "socket/socket_factory.hpp"
#include "stubs/test_tcp_server.hpp"

TEST(EpollerUnit, should_be_valid_after_construction) {
  Epoller epoller;
  EXPECT_TRUE(epoller.isValid());
}

TEST(EpollerUnit, should_return_true_when_add_called_with_valid_fd) {
  Epoller epoller;

  Network::ConnectedSockets sockets =
      Network::makeConnectedSockets(Ports::Epoller::ADD_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  ASSERT_TRUE(epoller.add(sockets.serverSide->getFd(), WatchFlags::READ));
  Network::closeSockets(sockets);
}
TEST(EpollerUnit, should_return_false_when_add_called_with_invalid_fd) {
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());
  EXPECT_FALSE(epoller.add(-1, WatchFlags::READ));
}

TEST(EpollerUnit, should_return_true_when_remove_called_on_watched_fd) {
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());
  Network::ConnectedSockets sockets =
      Network::makeConnectedSockets(Ports::Epoller::REMOVE_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  epoller.add(sockets.serverSide->getFd(), WatchFlags::READ);

  EXPECT_TRUE(epoller.remove(sockets.serverSide->getFd()));
  Network::closeSockets(sockets);
}

TEST(EpollerUnit, should_return_false_when_remove_called_on_unwatched_fd) {
  Epoller epoller;
  // ASSERT_TRUE(epoller.isValid());
  EXPECT_FALSE(epoller.remove(12345));
}

TEST(EpollerUnit, should_return_zero_event_on_timeout_when_no_data) {
  Epoller epoller;

  Network::ConnectedSockets sockets =
      Network::makeConnectedSockets(Ports::Epoller::TIMEOUT_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  epoller.add(sockets.serverSide->getFd(), WatchFlags::READ);
  std::vector<ReadyEvent> events;
  const int ready = epoller.wait(events, 0);
  EXPECT_EQ(ready, 0);
  EXPECT_TRUE(events.empty());

  epoller.remove(sockets.serverSide->getFd());
  Network::closeSockets(sockets);
}

TEST(EpollerIntegration, should_report_readable_event_when_data_arrives) {
  Epoller epoller;

  Network::ConnectedSockets sockets =
      Network::makeConnectedSockets(Ports::Epoller::READABLE_PORT);
  ASSERT_NE(sockets.serverSide, nullptr);
  ASSERT_NE(sockets.clientSide, nullptr);
  epoller.add(sockets.serverSide->getFd(), WatchFlags::READ);

  const std::vector<std::uint8_t> payload = {0xAB};
  ASSERT_TRUE(sockets.clientSide->send(payload).ok());

  std::vector<ReadyEvent> events;
  const int ready = epoller.wait(events, 100);
  ASSERT_GT(ready, 0);
  ASSERT_FALSE(events.empty());

  bool found = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == sockets.serverSide->getFd()) {
      found = true;
      EXPECT_TRUE(event.readable);
      EXPECT_FALSE(event.error);
      break;
    }
  }
  EXPECT_TRUE(found);

  epoller.remove(sockets.serverSide->getFd());
  Network::closeSockets(sockets);
}

TEST(EpollerIntegration, should_continue_after_one_fd_removed) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  Network::ConnectedSockets firstSockets =
      Network::makeConnectedSockets(Ports::Epoller::CONTINUE_PORT_1);
  Network::ConnectedSockets secondSockets =
      Network::makeConnectedSockets(Ports::Epoller::CONTINUE_PORT_2);
  ASSERT_NE(firstSockets.serverSide, nullptr);
  ASSERT_NE(secondSockets.serverSide, nullptr);
  ASSERT_NE(firstSockets.clientSide, nullptr);
  ASSERT_NE(secondSockets.clientSide, nullptr);

  epoller.add(firstSockets.serverSide->getFd(), WatchFlags::READ);
  epoller.add(secondSockets.serverSide->getFd(), WatchFlags::READ);

  const std::vector<std::uint8_t> firstPayload = {0x11};
  ASSERT_TRUE(firstSockets.clientSide->send(firstPayload).ok());

  std::vector<ReadyEvent> events;
  const int readyFirst = epoller.wait(events, 100);
  ASSERT_GT(readyFirst, 0);

  bool firstFound = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == firstSockets.serverSide->getFd()) {
      firstFound = true;
      EXPECT_TRUE(event.readable);
      break;
    }
  }
  EXPECT_TRUE(firstFound);

  epoller.remove(firstSockets.serverSide->getFd());
  Network::closeSockets(firstSockets);

  const std::vector<std::uint8_t> secondPayload = {0x22};
  ASSERT_TRUE(secondSockets.clientSide->send(secondPayload).ok());

  events.clear();
  const int readySecond = epoller.wait(events, 100);
  ASSERT_GT(readySecond, 0);

  bool secondFound = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == secondSockets.serverSide->getFd()) {
      secondFound = true;
      EXPECT_TRUE(event.readable);
      break;
    }
  }
  EXPECT_TRUE(secondFound);

  epoller.remove(secondSockets.serverSide->getFd());
  Network::closeSockets(secondSockets);
}

#endif
