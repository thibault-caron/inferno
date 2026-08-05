#include "dispatcher/agent_dispatcher.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// #include "socket/mock_socket_helpers.hpp"
#include <iostream>
#include <memory>

// #include "helpers_test.hpp"
#include "builders/frame_builder.hpp"
#include "builders/metrics_controller_test_factory.hpp"
#include "builders/os_info_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "stubs/fake_system_monitor.hpp"
#include "stubs/spy_socket.hpp"

class AgentDispatcherTest : public ::testing::Test {
 public:
  AgentDispatcherTest()
      : session(AgentSession(spy.makeUnique())),
        dispatcher(monitor),
        controller(MetricsControllerTestFactory::make(scrapperPtr)) {}

 protected:
  SpySocket spy;
  FakeSystemMonitor monitor;
  AgentSession session;
  AgentDispatcher dispatcher;
  FakeMetricsScrapper* scrapperPtr = nullptr;
  std::shared_ptr<MetricsController> controller;

  void SetUp() override { dispatcher.setMetricsController(controller); }
};

TEST_F(AgentDispatcherTest,
       should_get_os_info_with_matching_id_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      42, static_cast<std::uint8_t>(CommandType::OS_INFO), {});

  const ResponsePayload expected = {
      42, ResponseStatus::OK, CommandType::OS_INFO, 1, 0,
      ProtocolSerializer::serializeOsInfoPayload(OsInfoBuilder::create())};

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  EXPECT_EQ(response, expected);
}

TEST_F(AgentDispatcherTest, should_get_response_payload_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::OS_INFO), {});

  OsInfoPayload expected = FrameBuilder::makeOsInfoPayload();

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  const OsInfoPayload osInfo =
      ProtocolParser::parseOsInfoPayload(response.data);

  //------- Assert
  EXPECT_EQ(osInfo, expected);
}

TEST_F(AgentDispatcherTest,
       should_set_disconnect_request_to_true_and_send_nothing_on_disconnect) {
  //------- Act
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::DISCONNECT));

  //------- Assert
  EXPECT_TRUE(session.isDisconnectRequested());  // socket closed
  EXPECT_TRUE(spy.nothingSent());                // no reply to DISCONNECT
}

TEST_F(AgentDispatcherTest, should_get_process_list_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::RUNNING_PROCESSES), {});

  const std::vector<ProcessInfo> expectedProcesses =
      ProcessBuilder::createProcessInfoList();

  const ResponsePayload expected = {
      1, ResponseStatus::OK, CommandType::RUNNING_PROCESSES, 1, 0,
      ProtocolSerializer::serializeProcessInfoList(expectedProcesses)};

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  EXPECT_EQ(response, expected);

  const std::vector<ProcessInfo> processes =
      ProtocolParser::parseProcessInfoList(response.data);

  ASSERT_EQ(processes.size(), 3u);

  for (std::size_t i = 0; i < processes.size(); ++i) {
    EXPECT_EQ(processes[i], expectedProcesses[i]);
  }
}

TEST_F(AgentDispatcherTest, should_activate_metrics_controller_when_requested) {
  //------- Arrange
  const std::vector<std::uint8_t> cmd = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::START_METRICS), {});
  //------- Act
  dispatcher.handleFrame(session,
                         FrameBuilder::makeFrame(MessageType::COMMAND, cmd));
  //------- Assert
  EXPECT_TRUE(controller->isActive());
}

TEST_F(AgentDispatcherTest,
       should_deactivate_metrics_controller_when_requested) {
  //------- Arrange
  controller->start(session);

  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::STOP_METRICS), {});

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  EXPECT_FALSE(controller->isActive());
}

TEST_F(AgentDispatcherTest,
       should_send_multiple_frames_when_response_exceeds_chunk_size) {
  //------- Arrange
  monitor.processCount = 2340;  // enough to exceed one chunk

  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::RUNNING_PROCESSES), {});

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  SpySocket parserSpy;
  AgentSession parser(parserSpy.makeUnique());
  parser.appendToBuffer(spy.sent);

  std::vector<Frame> frames;
  std::optional<Frame> frame;
  while ((frame = parser.tryExtractFrame()).has_value()) {
    frames.push_back(*frame);
  }

  ASSERT_GE(frames.size(), 2u);
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const ResponsePayload chunk =
        ProtocolParser::parseResponsePayload(frames[i].payload);
    EXPECT_EQ(chunk.chunk_index, static_cast<std::uint8_t>(i));
    EXPECT_EQ(chunk.total_chunks, static_cast<std::uint8_t>(frames.size()));
  }
}

TEST_F(AgentDispatcherTest, should_execute_shell_command_when_requested) {
  //------- Arrange
  const std::string command = "echo hello";
  monitor.cannedShellOutput = "hello\n";

  const std::vector<std::uint8_t> payload = FrameBuilder::makeRawCommandPayload(
      1, static_cast<std::uint8_t>(CommandType::SHELL),
      {command.begin(), command.end()});

  const ResponsePayload expected =
      FrameBuilder::makeResponsePayload(1, CommandType::SHELL, "hello\n");

  //------- Act
  dispatcher.handleFrame(
      session, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  //------- Assert
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);

  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(spy.payload());

  EXPECT_EQ(response, expected);
  EXPECT_EQ(monitor.lastShellCommand, command);
}
