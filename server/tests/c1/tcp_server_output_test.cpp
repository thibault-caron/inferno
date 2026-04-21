#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "c1_test_helpers.hpp"
#include "tcp_server.hpp"

using ::testing::HasSubstr;

TEST(TcpServerOutput, should_print_parsed_register_details) {
  TcpServer server(0);

  const std::vector<std::uint8_t> frame = makeRegisterFrame("agent-print");
  server.appendToReceiveBuffer(frame.data(), frame.size());

  testing::internal::CaptureStdout();
  const std::size_t parsedCount = server.parseAvailableFrames();
  const std::string printed = testing::internal::GetCapturedStdout();

  EXPECT_EQ(parsedCount, 1);
  EXPECT_THAT(printed, HasSubstr("Parsed type=REGISTER"));
  EXPECT_THAT(printed, HasSubstr("hostname=agent-print"));
}
