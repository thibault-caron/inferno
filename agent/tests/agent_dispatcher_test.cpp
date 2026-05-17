#include "agent_dispatcher.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "helpers_test.hpp"
#include "protocol/protocol_parser.hpp"
// #include "socket/mock_socket_helpers.hpp"
#include "socket/spy_socket.hpp"
 
// ── AgentDispatcher tests ─────────────────────────────────────
// Three tests: happy path, disconnect handling, unknown command.
// The agent dispatcher is the mirror of the server dispatcher:
// it receives COMMANDs and sends RESPONSEs, not the other way.
// ─────────────────────────────────────────────────────────────
 
// ① Happy path — OS_INFO command arrives, response is sent back.
// Verifies: message type, response id matches command id, status OK,
// data is non-empty, chunk fields are correct for a single-chunk response.
TEST(AgentDispatcher,
     should_send_response_with_matching_id_on_os_info_command) {
  SpySocket spy;
  AgentSession session = makeSession(spy);
  AgentDispatcher dispatcher;
 
  CommandPayload cmd;
  cmd.id   = 42;
  cmd.type = CommandType::OS_INFO;
  cmd.data = "";
  const auto payload = ProtocolSerializer::serializeCommandPayload(cmd);
 
  dispatcher.handleFrame(session, makeFrame(MessageType::COMMAND, payload));
 
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::RESPONSE);
 
  const ResponsePayload rsp =
      ProtocolParser::parseResponsePayload(spy.payload());
  EXPECT_EQ(rsp.id, 42);                    // must echo the command id
  EXPECT_EQ(rsp.status, ResponseStatus::OK);
  EXPECT_EQ(rsp.total_chunks, 1);
  EXPECT_EQ(rsp.chunk_index, 0);
  EXPECT_FALSE(rsp.data.empty());           // agent sent something
}
 
// ② DISCONNECT — server sends it, agent must close the session.
// Verifies: socket is closed (isValid() false), nothing sent back.
// The agent must NOT reply to DISCONNECT — it just closes.
TEST(AgentDispatcher,
     should_close_session_and_send_nothing_on_disconnect) {
  SpySocket spy;
  AgentSession session = makeSession(spy);
  AgentDispatcher dispatcher;
 
  dispatcher.handleFrame(session, makeFrame(MessageType::DISCONNECT));
 
  EXPECT_FALSE(session.isValid());   // socket closed
  EXPECT_TRUE(spy.nothingSent());    // no reply to DISCONNECT
}
 
// ③ Unknown command — agent receives a CommandType it doesn't implement.
// Verifies: ERROR frame sent back, not a crash or silent ignore.
// Covers any future CommandType added to the enum before the agent handles it.
TEST(AgentDispatcher,
     should_send_error_response_when_command_type_is_not_implemented) {
  SpySocket spy;
  AgentSession session = makeSession(spy);
  AgentDispatcher dispatcher;
 
  // RUNNING_PROCESSES is declared in the protocol but not implemented yet
  CommandPayload cmd;
  cmd.id   = 1;
  cmd.type = CommandType::RUNNING_PROCESSES;
  cmd.data = "";
  const auto payload = ProtocolSerializer::serializeCommandPayload(cmd);
 
  dispatcher.handleFrame(session, makeFrame(MessageType::COMMAND, payload));
 
  ASSERT_GE(spy.sent.size(), static_cast<std::size_t>(LPTF_HEADER_SIZE));
  EXPECT_EQ(spy.messageType(), MessageType::ERROR);
}
