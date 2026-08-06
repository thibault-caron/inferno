#include "tcp_server.hpp"

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <future>
#include <optional>
#include <thread>

// #include "agent_session.hpp"
#include "builders/frame_builder.hpp"
#include "codec/protocol_parser.hpp"
#include "fixtures/common.hpp"
#include "fixtures/ports.hpp"
#include "socket/socket_factory.hpp"
#include "stubs/test_frame_transport.hpp"
#include "codec/protocol_serializer.hpp"

// ── Unit tests (no network) ───────────────────────────────────

TEST(TcpServerUnit,
     should_return_false_when_start_called_on_already_used_port) {
  // Raw socket intentional: we're testing that TcpServer detects a port
  // conflict that was created outside the codebase.
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(Ports::TcpServer::OCCUPIED_PORT);
  int occupier = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(occupier, -1);
  ASSERT_EQ(
      ::bind(occupier, reinterpret_cast<sockaddr*>(&address), sizeof(address)),
      0);

  TcpServer server(Ports::TcpServer::OCCUPIED_PORT);
  EXPECT_FALSE(server.start());
  ::close(occupier);
}

TEST(TcpServerUnit,
     should_return_nullptr_when_acceptAgent_called_before_start) {
  TcpServer server(Ports::TcpServer::NOT_STARTED_PORT);
  EXPECT_EQ(server.acceptAgent(), nullptr);
}

// ── Integration tests (real loopback) ────────────────────────

TEST(TcpServerIntegration,
     should_return_true_when_start_is_called_on_available_port) {
  TcpServer server(Ports::TcpServer::AVAILABLE_PORT);
  EXPECT_TRUE(server.start());
}

TEST(TcpServerIntegration,
     should_return_false_when_start_is_called_a_second_time) {
  TcpServer server(Ports::TcpServer::DOUBLE_START_PORT);
  server.start();
  EXPECT_FALSE(server.start());
}

TEST(TcpServerIntegration, should_return_valid_socket_when_agent_connects) {
  const std::uint16_t port = Ports::TcpServer::AGENT_CONNECT_PORT;
  TcpServer server(port);
  server.start();

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture = serverReady.get_future().share();

  std::thread connector([&] {
    serverReadyFuture.wait();
    std::unique_ptr<ISocket> agentSocket = SocketFactory::createTCP();
    if (agentSocket) {
      agentSocket->connect(Common::SERVER_HOST, port);
    }
  });

  serverReady.set_value();
  auto accepted = server.acceptAgent();
  connector.join();

  EXPECT_NE(accepted, nullptr);
  EXPECT_TRUE(accepted->isValid());
}

TEST(TcpServerIntegration,
     should_receive_and_echo_frame_sent_by_connected_agent) {
  const std::uint16_t port = Ports::TcpServer::ECHO_PORT;
  TcpServer server(port);
  server.start();  // setup

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture = serverReady.get_future().share();
  std::promise<std::optional<ResponsePayload>> responsePromise;
  std::future<std::optional<ResponsePayload>> responseFuture =
      responsePromise.get_future();

  // ── Agent thread ──────────────────────────────────────────
  std::thread agentThread([&] {
    serverReadyFuture.wait();
    std::optional<ResponsePayload> response;

    std::unique_ptr<ISocket> agentSocket = SocketFactory::createTCP();
    if (agentSocket && agentSocket->connect(Common::SERVER_HOST, port)) {
      // AgentSession agentSession(std::move(agentSocket));
      TestFrameTransport agentConnection(std::move(agentSocket));

      // Send REGISTER using the shared helper
      // const auto registerFrame =
      //     makeRawFrame(MessageType::REGISTER,
      //     FrameBuilder::makeRawOsInfoPayload());
      try {
        Frame frame = FrameBuilder::makeFrame(
            MessageType::REGISTER, FrameBuilder::makeRawOsInfoPayload());
        agentConnection.sendFrame(frame);
        // send succeeded, continue with the rest
        agentConnection.receiveIntoBuffer();
        std::optional<Frame> received = agentConnection.tryExtractFrame();
        if (received && received->header.type == MessageType::RESPONSE) {
          response = ProtocolParser::parseResponsePayload(received->payload);
        }
      } catch (const std::exception&) {
        // send failed, response stays nullopt
      }
      // if (agentSession.send(registerFrame).ok()) {
      //   agentSession.receiveIntoBuffer();
      //   std::optional<Frame> frame = agentSession.tryExtractFrame();
      //   if (frame && frame->header.type == MessageType::RESPONSE) {
      //     response = ProtocolParser::parseResponsePayload(frame->payload);
      //   }
      // }
    }
    responsePromise.set_value(response);
  });

  // ── Server side ───────────────────────────────────────────
  serverReady.set_value();
  auto serverSocket = server.acceptAgent();
  ASSERT_NE(serverSocket, nullptr);
  // AgentSession serverSession(std::move(serverSocket));
  TestFrameTransport serverSession(std::move(serverSocket));

  // Read REGISTER
  serverSession.receiveIntoBuffer();
  std::optional<Frame> registerFrame = serverSession.tryExtractFrame();
  ASSERT_TRUE(registerFrame.has_value());
  EXPECT_EQ(registerFrame->header.type, MessageType::REGISTER);

  // Send RESPONSE using the shared helper
  const Frame responseFrame = FrameBuilder::makeFrame(
      MessageType::RESPONSE,
      ProtocolSerializer::serializeResponsePayload(
          FrameBuilder::makeResponsePayload(7, CommandType::OS_INFO,
                                            "pong")));
  // FrameBuilder::makeRawResponsePayload(7, Common::bytesFromString("pong")));
  ASSERT_NO_THROW(serverSession.sendFrame(responseFrame));

  agentThread.join();

  const std::optional<ResponsePayload> response = responseFuture.get();
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->id, 7);
  EXPECT_EQ(response->status, ResponseStatus::OK);
  // EXPECT_EQ(response->data, ProtocolSerializer::toBytes("pong"));
}

TEST(TcpServerIntegration, should_report_loopback_address_for_connected_agent) {
  const std::uint16_t port = Ports::TcpServer::REMOTE_ADDR_PORT;
  TcpServer server(port);
  server.start();  // setup

  std::promise<void> serverReady;
  std::shared_future<void> serverReadyFuture = serverReady.get_future().share();
  std::promise<void> serverDone;
  std::shared_future<void> serverDoneFuture = serverDone.get_future().share();

  std::thread connector([&] {
    serverReadyFuture.wait();
    std::unique_ptr<ISocket> agentSocket = SocketFactory::createTCP();
    if (agentSocket) {
      agentSocket->connect(Common::SERVER_HOST, port);
      serverDoneFuture
          .wait();  // keep socket alive until server has read the address
    }
  });

  serverReady.set_value();
  auto accepted = server.acceptAgent();
  ASSERT_NE(accepted, nullptr);
  EXPECT_EQ(accepted->remoteAddress(), "::ffff:" + Common::SERVER_HOST);
  EXPECT_GT(accepted->remotePort(), 0);

  serverDone.set_value();
  connector.join();
}