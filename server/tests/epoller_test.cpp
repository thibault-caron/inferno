#include "poller/epoller.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#if !defined(__linux__)

TEST(Epoller, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Epoller is Linux-only";
}

#else

#include <unistd.h>

namespace {

struct PipeFds {
  int readFd = -1;
  int writeFd = -1;
};

PipeFds makePipe() {
  int fds[2] = {-1, -1};
  if (::pipe(fds) != 0) {
    return {};
  }
  return {fds[0], fds[1]};
}

void closePipe(const PipeFds& pipeFds) {
  if (pipeFds.readFd != -1) {
    ::close(pipeFds.readFd);
  }
  if (pipeFds.writeFd != -1) {
    ::close(pipeFds.writeFd);
  }
}

}  // namespace

TEST(EpollerUnit, should_be_valid_after_construction) {
  Epoller epoller;
  EXPECT_TRUE(epoller.isValid());
}

TEST(EpollerUnit, should_return_false_when_add_called_with_invalid_fd) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());
  EXPECT_FALSE(epoller.add(-1, WatchFlags::READ));
}

TEST(EpollerUnit, should_return_false_when_remove_called_on_unwatched_fd) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());
  EXPECT_FALSE(epoller.remove(12345));
}

TEST(EpollerUnit, should_return_zero_on_timeout_when_no_data) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  const PipeFds pipeFds = makePipe();
  ASSERT_NE(pipeFds.readFd, -1);
  ASSERT_NE(pipeFds.writeFd, -1);

  ASSERT_TRUE(epoller.add(pipeFds.readFd, WatchFlags::READ));

  std::vector<ReadyEvent> events;
  const int ready = epoller.wait(events, 0);
  EXPECT_EQ(ready, 0);
  EXPECT_TRUE(events.empty());

  epoller.remove(pipeFds.readFd);
  closePipe(pipeFds);
}

TEST(EpollerIntegration, should_report_readable_event_when_data_arrives) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  const PipeFds pipeFds = makePipe();
  ASSERT_NE(pipeFds.readFd, -1);
  ASSERT_NE(pipeFds.writeFd, -1);

  ASSERT_TRUE(epoller.add(pipeFds.readFd, WatchFlags::READ));

  const std::uint8_t byte = 0xAB;
  ASSERT_EQ(::write(pipeFds.writeFd, &byte, 1), 1);

  std::vector<ReadyEvent> events;
  const int ready = epoller.wait(events, 100);
  ASSERT_GT(ready, 0);
  ASSERT_FALSE(events.empty());

  bool found = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == pipeFds.readFd) {
      found = true;
      EXPECT_TRUE(event.readable);
      EXPECT_FALSE(event.error);
      break;
    }
  }
  EXPECT_TRUE(found);

  epoller.remove(pipeFds.readFd);
  closePipe(pipeFds);
}

TEST(EpollerIntegration, should_continue_after_one_fd_removed) {
  Epoller epoller;
  ASSERT_TRUE(epoller.isValid());

  const PipeFds firstPipe = makePipe();
  const PipeFds secondPipe = makePipe();
  ASSERT_NE(firstPipe.readFd, -1);
  ASSERT_NE(firstPipe.writeFd, -1);
  ASSERT_NE(secondPipe.readFd, -1);
  ASSERT_NE(secondPipe.writeFd, -1);

  ASSERT_TRUE(epoller.add(firstPipe.readFd, WatchFlags::READ));
  ASSERT_TRUE(epoller.add(secondPipe.readFd, WatchFlags::READ));

  const std::uint8_t firstByte = 0x11;
  ASSERT_EQ(::write(firstPipe.writeFd, &firstByte, 1), 1);

  std::vector<ReadyEvent> events;
  const int readyFirst = epoller.wait(events, 100);
  ASSERT_GT(readyFirst, 0);

  bool firstFound = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == firstPipe.readFd) {
      firstFound = true;
      EXPECT_TRUE(event.readable);
      break;
    }
  }
  EXPECT_TRUE(firstFound);

  EXPECT_TRUE(epoller.remove(firstPipe.readFd));
  closePipe(firstPipe);

  const std::uint8_t secondByte = 0x22;
  ASSERT_EQ(::write(secondPipe.writeFd, &secondByte, 1), 1);

  events.clear();
  const int readySecond = epoller.wait(events, 100);
  ASSERT_GT(readySecond, 0);

  bool secondFound = false;
  for (const auto& event : events) {
    if (event.fileDescriptor == secondPipe.readFd) {
      secondFound = true;
      EXPECT_TRUE(event.readable);
      break;
    }
  }
  EXPECT_TRUE(secondFound);

  epoller.remove(secondPipe.readFd);
  closePipe(secondPipe);
}

#endif
