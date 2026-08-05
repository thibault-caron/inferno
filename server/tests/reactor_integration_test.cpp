#include <gtest/gtest.h>

#include <future>
#include <optional>
#include <thread>

// #include "agent_session.hpp"
#include "agent_connection.hpp"
#include "builders/frame_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "dispatcher/server_dispatcher.hpp"
#include "fixtures/common.hpp"
#include "fixtures/ports.hpp"
#include "poller/epoller.hpp"
#include "protocol/lptf_protocol.hpp"
#include "reactor.hpp"
#include "service/agent_service.hpp"
#include "service/command_service.hpp"
#include "service/response_service.hpp"
#include "service/metrics_service.hpp"
#include "session_manager.hpp"
#include "socket/socket_factory.hpp"
#include "stubs/fake_agent_repository.hpp"
#include "stubs/fake_command_repository.hpp"
#include "stubs/fake_response_repository.hpp"
#include "stubs/fake_metrics_repository.hpp"
#include "stubs/spy_socket.hpp"
#include "tcp_server.hpp"

#if !defined(__linux__)

TEST(ReactorIntegration, NotSupportedOnThisPlatform) {
  GTEST_SKIP() << "Reactor epoll integration is Linux-only";
}

#else

#include <unistd.h>

class ReactorIntegrationTest : public ::testing::Test {
 public:
  // ReactorIntegrationTest()
  //     : poller((spy.makeUnique())), dispatcher(manager) {}
  // reactor(server, dispatcher, epoller, manager) {}

 protected:
  Epoller epoller;
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
        std::make_unique<ResponseService>(*fakeResponseRepo);
    responseService = responseServiceUnique.get();

    // Metrics
    metricsRepoUnique = std::make_unique<FakeMetricsRepository>();
    fakeMetricsRepo = metricsRepoUnique.get();

    metricsServiceUnique =
        std::make_unique<MetricsService>(*fakeMetricsRepo, manager);
    metricsService = metricsServiceUnique.get();

    dispatcher.emplace(manager, *agentService, *commandService, *responseService,
                       *metricsService);
  }

  // Reactor reactor;

  std::thread startReactorThread(TcpServer& server, Reactor& reactor,
                                 std::promise<void>& reactorReady) {
    server.start();
    server.setNonBlocking();
    return std::thread([&reactor, &reactorReady] {
      reactorReady.set_value();
      reactor.run();
    });
  }

  // std::thread startDashboardThread(ISocket& dashboard, std::uint16_t port,
  //                                  std::promise<void>& dashboardReady) {
  //   dashboard.connect(Common::SERVER_HOST, port);

  //   OsInfoPayload info = FrameBuilder::makeOsInfoPayload();
  //   std::vector<std::uint8_t> payload =
  //       ProtocolSerializer::serializeOsInfoPayload(info);

  //   Frame frame;
  //   frame.header =
  //       ProtocolHelper::createHeader(MessageType::DASHBOARD_REGISTER,
  //       payload);
  //   frame.payload = payload;
  //   std::vector<std::uint8_t> finalPayload =
  //       ProtocolSerializer::serializeFrame(frame);

  //   EXPECT_TRUE(socket->send(registerFrame).ok());
  //   dashboardReady.set_value();
  // }
};

// ① Happy path — agent connects and sends REGISTER.
TEST_F(ReactorIntegrationTest, should_accept_register_without_error) {
  const std::uint16_t port = Ports::Reactor::HAPPY_PATH_PORT;
  TcpServer server(port);
  Reactor reactor(server, *dispatcher, epoller, manager);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  auto socket = SocketFactory::createTCP();
  ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));

  const auto registerFrame = FrameBuilder::makeRawFrame(
      MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
  EXPECT_TRUE(socket->send(registerFrame).ok());

  reactor.stop();
  reactorThread.join();
}

// TODO test commented out , need to work on it
// ② Protocol enforcement — agent sends COMMAND before REGISTER → ERROR back.
// TEST_F(ReactorIntegrationTest,
//        should_send_error_when_first_message_is_not_register) {
//   const std::uint16_t port = Ports::Reactor::INVALID_FIRST_MESSAGE_PORT;
//   TcpServer server(port);
//   Reactor reactor(server, *dispatcher, epoller, manager);

//   std::promise<void> reactorReady;
//   auto reactorThread = startReactorThread(server, reactor, reactorReady);
//   reactorReady.get_future().wait();

//   auto socket = SocketFactory::createTCP();
//   ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));

//   const auto commandFrame = FrameBuilder::makeRawFrame(MessageType::COMMAND);
//   ASSERT_TRUE(socket->send(commandFrame).ok());

//   AgentConnection session(std::move(socket));
//   session.receiveIntoBuffer();
//   std::optional<Frame> frame = session.tryExtractFrame();
//   ASSERT_TRUE(frame.has_value());
//   EXPECT_EQ(frame->header.type, MessageType::INFERNO_ERROR);

//   reactor.stop();
//   session.close();
//   reactorThread.join();
// }

// ③ Resilience — first agent disconnects, second agent connects and registers.
// The second connection is made without sleeping: the OS queues it in the
// backlog immediately after the first socket closes, and the reactor drains
// both events (disconnect + new connection) on the next epoll_wait.
TEST_F(ReactorIntegrationTest,
       should_keep_serving_after_first_agent_disconnects) {
  const std::uint16_t port = Ports::Reactor::DISCONNECT_PORT;
  TcpServer server(port);
  Reactor reactor(server, *dispatcher, epoller, manager);

  std::promise<void> reactorReady;
  auto reactorThread = startReactorThread(server, reactor, reactorReady);
  reactorReady.get_future().wait();

  // First agent — scope exit closes the socket, reactor sees EPOLLHUP
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));
    const auto registerFrame = FrameBuilder::makeRawFrame(
        MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
    EXPECT_TRUE(socket->send(registerFrame).ok());
  }

  // Second agent — connects immediately; OS queues it in the backlog
  {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket->connect(Common::SERVER_HOST, port));
    const auto registerFrame = FrameBuilder::makeRawFrame(
        MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
    EXPECT_TRUE(socket->send(registerFrame).ok());
  }

  reactor.stop();
  reactorThread.join();
}

#endif