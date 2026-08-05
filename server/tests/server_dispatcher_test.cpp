#include "dispatcher/server_dispatcher.hpp"

#include <gtest/gtest.h>

#include "agent_connection.hpp"
#include "builders/frame_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "fixtures/common.hpp"
#include "service/agent_service.hpp"
#include "service/command_service.hpp"
#include "service/metrics_service.hpp"
#include "service/response_service.hpp"
#include "session_manager.hpp"
#include "stubs/fake_agent_repository.hpp"
#include "stubs/fake_command_repository.hpp"
#include "stubs/fake_metrics_repository.hpp"
#include "stubs/fake_response_repository.hpp"
#include "stubs/spy_socket.hpp"

class ServerDispatcherTest : public ::testing::Test {
 protected:
  SessionManager manager;
  // Agent
  std::unique_ptr<FakeAgentRepository> agentRepoUnique;
  std::unique_ptr<AgentService> agentServiceUnique;
  FakeAgentRepository* fakeAgentRepo = nullptr;
  IAgentService* agentService = nullptr;

  // Command
  std::unique_ptr<FakeCommandRepository> commandRepoUnique;
  std::unique_ptr<CommandService> commandServiceUnique;
  FakeCommandRepository* fakeCommandRepo = nullptr;
  ICommandService* commandService = nullptr;

  // Response
  std::unique_ptr<FakeResponseRepository> responseRepoUnique;
  std::unique_ptr<ResponseService> responseServiceUnique;
  FakeResponseRepository* fakeResponseRepo = nullptr;
  IResponseService* responseService = nullptr;

  // Metrics
  std::unique_ptr<FakeMetricsRepository> metricsRepoUnique;
  std::unique_ptr<MetricsService> metricsServiceUnique;
  FakeMetricsRepository* fakeMetricsRepo = nullptr;
  IMetricsService* metricsService = nullptr;

  std::optional<ServerDispatcher> dispatcher;

  void SetUp() override {
    // Agent
    agentRepoUnique = std::make_unique<FakeAgentRepository>();
    fakeAgentRepo = agentRepoUnique.get();

    agentServiceUnique =
        std::make_unique<AgentService>(*fakeAgentRepo, manager);
    agentService = agentServiceUnique.get();

    // Command
    commandRepoUnique = std::make_unique<FakeCommandRepository>();
    fakeCommandRepo = commandRepoUnique.get();

    commandServiceUnique =
        std::make_unique<CommandService>(*fakeCommandRepo, manager);
    commandService = commandServiceUnique.get();

    // Response
    responseRepoUnique = std::make_unique<FakeResponseRepository>();
    fakeResponseRepo = responseRepoUnique.get();

    responseServiceUnique =
        std::make_unique<ResponseService>(*fakeResponseRepo, *commandService);
    responseService = responseServiceUnique.get();

    // Metrics
    metricsRepoUnique = std::make_unique<FakeMetricsRepository>();
    fakeMetricsRepo = metricsRepoUnique.get();

    metricsServiceUnique =
        std::make_unique<MetricsService>(*fakeMetricsRepo, manager);
    metricsService = metricsServiceUnique.get();

    dispatcher.emplace(manager, *agentService, *commandService,
                       *responseService, *metricsService);
  }

  // Helper: Create an agent through the manager (mirrors
  // Reactor::onNewConnection)
  AgentConnection& createAgent(int fd, SpySocket& spy) {
    spy.setFd(fd);
    manager.addAgent(fd, spy.makeUnique());

    return manager.getAgent(fd);
  }

  // Helper: Create and register a dashboard through the manager
  AgentConnection& createDashboard(int fd, SpySocket& spy) {
    spy.setFd(fd);
    manager.addAgent(fd, spy.makeUnique());
    manager.setDashboardFd(fd);
    AgentConnection& dashboard = manager.getDashboard();
    // Dashboard doesn't call recordAgentTarget (it's not a routing target)
    return dashboard;
  }
};

// ════════════════════════════════════════════════════════════════════════════
// Basic Agent Registration Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest, should_register_session_on_register) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  // Act
  dispatcher->handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::REGISTER,
                                     FrameBuilder::makeRawOsInfoPayload()));

  // Assert
  EXPECT_TRUE(agent.getIsRegistered());
  EXPECT_EQ(agent.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
}

// TODO sendError method on frame transport now, not on dispatcher, only called
// if dashboard exist
TEST_F(ServerDispatcherTest,
       should_send_error_when_unknown_message_type_received) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  // Act
  // COMMAND is server→agent — receiving it is a protocol violation
  dispatcher->handleFrame(
      agent,
      FrameBuilder::makeFrame(
          MessageType::COMMAND));  // if an agent sends a command, it should
                                   // send an error!

  // Assert
  ASSERT_GE(agentSpy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(agentSpy.messageType(), MessageType::INFERNO_ERROR);
}

// TODO now db assign command id, no need for this anymore?
// TEST_F(ServerDispatcherTest,
//        should_increment_command_id_across_successive_commands) {
//   // Arrange
//   SpySocket agentSpy;
//   AgentConnection& agent = createAgent(101, agentSpy);
//   std::vector<std::uint16_t> ids;

//   // Act
//   for (int i = 0; i < 3; ++i) {
//     CommandPayload command;
//     command.data = "";
//     command.id = 1;
//     command.type = CommandType::OS_INFO;
//     dispatcher->sendCommand(agent, command);

//     // feed sent bytes into receive buffer
//     agent.appendToBuffer(agentSpy.sent);

//     // clear spy so each iteration is isolated
//     agentSpy.sent.clear();

//     // extract frames via real RX pipeline
//     while (auto frame = agent.tryExtractFrame()) {
//       CommandPayload cmd =
//       ProtocolParser::parseCommandPayload(frame->payload);
//       ids.push_back(cmd.id);
//     }
//   }

//   // Assert
//   ASSERT_EQ(ids.size(), 3);
//   EXPECT_EQ(ids[0], 1);
//   EXPECT_EQ(ids[1], 2);
//   EXPECT_EQ(ids[2], 3);
// }

// ════════════════════════════════════════════════════════════════════════════
// Dashboard Registration Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest, should_register_dashboard_on_dashboard_register) {
  // Arrange
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);

  // Act
  dispatcher->handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::DASHBOARD_REGISTER,
                                         FrameBuilder::makeRawOsInfoPayload()));

  // Assert
  EXPECT_TRUE(dashboard.getIsRegistered());
  EXPECT_EQ(dashboard.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
}

TEST_F(ServerDispatcherTest,
       should_send_agents_list_when_dashboard_registers_empty) {
  // Arrange
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);

  // Act - Dashboard registers with no agents connected
  dispatcher->handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::DASHBOARD_REGISTER,
                                         FrameBuilder::makeRawOsInfoPayload()));

  // Assert - A DATA frame should be sent
  ASSERT_GE(dashboardSpy.sent.size(),
            static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::DATA);

  // Verify payload is AGENTS subtype with empty data
  const DashboardData data =
      ProtocolParser::parseDashboardData(dashboardSpy.payload());
  EXPECT_EQ(data.data.subtype, DataType::AGENTS);
  EXPECT_EQ(data.data.data.size(), 2);  // No agents connected

  uint16_t agentCount = (data.data.data[0] << 8) | data.data.data[1];
  EXPECT_EQ(agentCount, 0);
}

// ════════════════════════════════════════════════════════════════════════════
// Agent Registration with Connected Dashboard Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest,
       should_notify_dashboard_when_agent_registers_after_dashboard) {
  // Arrange: Dashboard already connected
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);
  dashboard.setId("DA:SH:BO:AR:D_:ID");
  // Clear initial AGENTS list message from dashboard registration
  dashboardSpy.sent.clear();

  // Act: Agent registers
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  dispatcher->handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::REGISTER,
                                     FrameBuilder::makeRawOsInfoPayload()));

  // Assert: Dashboard received REGISTRATION notification
  ASSERT_GE(dashboardSpy.sent.size(),
            static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::DATA);

  // Verify it's a REGISTRATION subtype
  const DashboardData data =
      ProtocolParser::parseDashboardData(dashboardSpy.payload());
  EXPECT_EQ(data.data.subtype, DataType::REGISTRATION);
}

TEST_F(ServerDispatcherTest,
       should_not_crash_when_agent_registers_without_dashboard) {
  // Arrange
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);

  // Act - Agent registers with no dashboard connected
  dispatcher->handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::REGISTER,
                                     FrameBuilder::makeRawOsInfoPayload()));

  // Assert - Should register successfully even without dashboard
  EXPECT_TRUE(agent.getIsRegistered());
  EXPECT_EQ(agent.getAgentInfo().hostname, Protocol::TEST_HOSTNAME_STR);
}

TEST_F(ServerDispatcherTest,
       should_send_agents_list_when_dashboard_registers_with_agents) {
  // Arrange: Setup two agents first
  SpySocket agent1Spy;
  AgentConnection& agent1 = createAgent(101, agent1Spy);
  agent1.setAgentInfo(FrameBuilder::makeOsInfoPayload());
  agent1.setIsRegisered();
  agent1.setId("AG:EN:T1:ID:00:00");
  // "AG:EN:T1:ID:00:00"

  SpySocket agent2Spy;
  AgentConnection& agent2 = createAgent(102, agent2Spy);
  agent2.setAgentInfo(FrameBuilder::makeOsInfoPayload());
  agent2.setIsRegisered();
  agent2.setId("AG:EN:T2:ID:00:00");

  // Act: Dashboard registers
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);

  dispatcher->handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::DASHBOARD_REGISTER,
                                         FrameBuilder::makeRawOsInfoPayload()));

  // Assert: Dashboard received AGENTS with 2 agents
  ASSERT_GE(dashboardSpy.sent.size(),
            static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::DATA);

  const DashboardData data =
      ProtocolParser::parseDashboardData(dashboardSpy.payload());
  EXPECT_EQ(data.data.subtype, DataType::AGENTS);
  EXPECT_GT(data.data.data.size(), 0);  // Has agent data
}

// ════════════════════════════════════════════════════════════════════════════
// Dashboard Command Routing Tests
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ServerDispatcherTest, should_route_dashboard_command_to_agent) {
  // Arrange: Setup agent with target tracking
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);
  agent.setAgentInfo(FrameBuilder::makeOsInfoPayload());
  agent.setIsRegisered();
  agent.setId("AA:BB:CC:DD:EE:FF");
  manager.recordAgentTarget(101, "AA:BB:CC:DD:EE:FF");

  // Setup dashboard
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);
  dashboard.setIsRegisered();
  dashboardSpy.sent.clear();

  // Act: Dashboard sends OS_INFO command targeting agent
  DashboardCommand cmd;
  cmd.target = "AA:BB:CC:DD:EE:FF";
  cmd.command.id = 0;  // Dashboard doesn't set ID
  cmd.command.type = CommandType::OS_INFO;
  cmd.command.data = "";

  std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeDashboardCommand(cmd);
  dispatcher->handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  // Assert: Agent received command with generated ID
  ASSERT_GE(agentSpy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(agentSpy.messageType(), MessageType::COMMAND);

  CommandPayload received =
      ProtocolParser::parseCommandPayload(agentSpy.payload());
  EXPECT_EQ(received.type, CommandType::OS_INFO);
  EXPECT_NE(received.id, 0);  // Server generated new ID // TODO check why first
                              // command id = 0 in server
}

TEST_F(ServerDispatcherTest,
       should_send_error_when_dashboard_targets_nonexistent_agent) {
  // Arrange: Dashboard only, no agents
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);
  dashboard.setIsRegisered();
  dashboardSpy.sent.clear();

  // Act: Dashboard targets non-existent agent
  DashboardCommand cmd;
  // "AA:BB:CC:DD:EE:FF"
  cmd.target = "noexistent:AA:BB";
  cmd.command.type = CommandType::OS_INFO;
  cmd.command.data = "";

  std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeDashboardCommand(cmd);
  dispatcher->handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::COMMAND, payload));

  // Assert: Dashboard got error
  ASSERT_GE(dashboardSpy.sent.size(),
            static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::INFERNO_ERROR);
}

// test response flow
TEST_F(ServerDispatcherTest, should_forward_agent_response_to_dashboard) {
  // Arrange: Setup agent with target tracking
  SpySocket agentSpy;
  AgentConnection& agent = createAgent(101, agentSpy);
  agent.setAgentInfo(FrameBuilder::makeOsInfoPayload());
  agent.setIsRegisered();
  agent.setId("AA:BB:CC:DD:EE:FF");
  manager.recordAgentTarget(101, "AA:BB:CC:DD:EE:FF");

  // Setup dashboard
  SpySocket dashboardSpy;
  AgentConnection& dashboard = createDashboard(100, dashboardSpy);
  dashboard.setIsRegisered();
  dashboardSpy.sent.clear();

  // Act 1: Dashboard sends OS_INFO command targeting agent
  DashboardCommand cmd;
  cmd.target = "AA:BB:CC:DD:EE:FF";
  cmd.command.id = 0;  // Dashboard doesn't set ID
  cmd.command.type = CommandType::OS_INFO;
  cmd.command.data = "";

  std::vector<std::uint8_t> cmdPayload =
      ProtocolSerializer::serializeDashboardCommand(cmd);
  dispatcher->handleFrame(
      dashboard, FrameBuilder::makeFrame(MessageType::COMMAND, cmdPayload));

  // Capture the command ID that was generated by server
  ASSERT_GE(agentSpy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  CommandPayload receivedCmd =
      ProtocolParser::parseCommandPayload(agentSpy.payload());
  uint32_t cmdId = receivedCmd.id;

  // Clear spies for response phase
  agentSpy.sent.clear();
  dashboardSpy.sent.clear();

  // Act 2: Agent sends RESPONSE back (with same command ID)
  ResponsePayload response;
  response.id = cmdId;
  response.type = CommandType::OS_INFO;
  response.status = ResponseStatus::OK;  // OK
  response.total_chunks = 1;
  response.chunk_index = 0;
  // response.data = {'O', 'K'};
  response.data = ProtocolSerializer::serializeOsInfoPayload(
      FrameBuilder::makeOsInfoPayload());

  std::vector<std::uint8_t> responsePayload =
      ProtocolSerializer::serializeResponsePayload(response);
  dispatcher->handleFrame(
      agent, FrameBuilder::makeFrame(MessageType::RESPONSE, responsePayload));

  // Assert: Dashboard received a RESPONSE frame
  ASSERT_GE(dashboardSpy.sent.size(),
            static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(dashboardSpy.messageType(), MessageType::RESPONSE);

  // Parse the DashboardResponse from dashboard spy
  DashboardResponse dashResponse =
      ProtocolParser::parseDashboardResponse(dashboardSpy.payload());

  // Verify target and response match
  EXPECT_EQ(dashResponse.target, "AA:BB:CC:DD:EE:FF");
  EXPECT_EQ(dashResponse.response.id, cmdId);
  EXPECT_EQ(dashResponse.response.status, ResponseStatus::OK);
}