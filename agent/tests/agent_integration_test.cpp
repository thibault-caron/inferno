#include <gtest/gtest.h>
 
#include <atomic>
#include <optional>
#include <thread>
 
#include "agent_dispatcher.hpp"
#include "agent_session.hpp"
#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/socket_factory.hpp"
#include "test_tcp_server.hpp"
#include "test_constants.hpp"
 
// Full cycle: REGISTER → COMMAND(OS_INFO) → RESPONSE → DISCONNECT.
//
// Both sides use shared/ classes only:
//   server role  — TestTcpServer + AgentSession (same framing path as Reactor)
//   agent role   — real agent code, unchanged
//
// Thread layout:
//   main thread  — server role: accept, read frames, send commands
//   agent thread — agent role:  connect, register, dispatch
TEST(AgentIntegration, should_register_respond_and_disconnect) {
  const std::uint16_t port = TestConstants::AGENT_INTEGRATION_PORT;
 
  TestTcpServer server(port);
  ASSERT_TRUE(server.start());
 
  std::atomic<bool> agentExited{false};
 
  // ── Agent thread — real agent code, untouched ─────────────
  std::thread agentThread([&] {
    auto socket = SocketFactory::createTCP();
    ASSERT_TRUE(socket && socket->connect("127.0.0.1", port));
 
    AgentSession session(std::move(socket));
    AgentDispatcher dispatcher;
    dispatcher.sendRegister(session);
 
    while (session.isValid()) {
      const SocketResult result = session.receiveIntoBuffer();
      if (!result.ok() || result.bytesTransferred <= 0) break;
 
      std::optional<Frame> frame;
      while (session.isValid() && (frame = session.tryExtractFrame())) {
        dispatcher.handleFrame(session, frame.value());
      }
    }
    agentExited = true;
  });
 
  // ── Server role — accept into AgentSession, same path as Reactor ──
  std::unique_ptr<ISocket> serverSocket = server.acceptAgent();
  ASSERT_NE(serverSocket, nullptr);
  AgentSession serverSession(std::move(serverSocket));
 
  // ── Read REGISTER ─────────────────────────────────────────
  serverSession.receiveIntoBuffer();
  std::optional<Frame> registerFrame = serverSession.tryExtractFrame();
  ASSERT_TRUE(registerFrame.has_value());
  EXPECT_EQ(registerFrame->header.type, MessageType::REGISTER);
 
  const RegisterPayload reg =
      ProtocolParser::parseRegisterPayload(registerFrame->payload);
  EXPECT_EQ(reg.hostname, "inferno-agent");
 
  // ── Send COMMAND(OS_INFO) ─────────────────────────────────
  CommandPayload cmd;
  cmd.id   = 0;
  cmd.type = CommandType::OS_INFO;
  cmd.data = "";
  const auto cmdPayload = ProtocolSerializer::serializeCommandPayload(cmd);
  ASSERT_TRUE(serverSession.send(makeRawFrame(MessageType::COMMAND,
                                              cmdPayload)).ok());
 
  // ── Read RESPONSE ─────────────────────────────────────────
  serverSession.receiveIntoBuffer();
  std::optional<Frame> responseFrame = serverSession.tryExtractFrame();
  ASSERT_TRUE(responseFrame.has_value());
  EXPECT_EQ(responseFrame->header.type, MessageType::RESPONSE);
 
  const ResponsePayload rsp =
      ProtocolParser::parseResponsePayload(responseFrame->payload);
  EXPECT_EQ(rsp.id, 0);
  EXPECT_EQ(rsp.status, ResponseStatus::OK);
  EXPECT_EQ(rsp.data, "hello world from agent");
 
  // ── Send DISCONNECT — agent must close cleanly ────────────
  ASSERT_TRUE(serverSession.send(makeRawFrame(MessageType::DISCONNECT)).ok());
 
  agentThread.join();
  EXPECT_TRUE(agentExited);
}
