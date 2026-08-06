#ifndef PORTS_FIXTURE_HPP
#define PORTS_FIXTURE_HPP

#include <cstdint>

// Port range reserved for INFERNO tests: 9876, 19872–19999
// Each test file owns a fixed sub-range to avoid conflicts.
// Do NOT reuse ports across test files.
namespace Ports {
// ── tcp_server_test.cpp ──────────────────────────────────────
namespace TcpServer {
constexpr std::uint16_t AVAILABLE_PORT = 19872;      // start() succeeds
constexpr std::uint16_t DOUBLE_START_PORT = 19873;   // start() called twice
constexpr std::uint16_t OCCUPIED_PORT = 19876;       // port already taken
constexpr std::uint16_t NOT_STARTED_PORT = 19877;    // accept before start
constexpr std::uint16_t AGENT_CONNECT_PORT = 19879;  // single agent connects
constexpr std::uint16_t ECHO_PORT = 19880;           // echo data back
constexpr std::uint16_t REMOTE_ADDR_PORT = 19881;    // check remote addr/port
}  // namespace TcpServer

// ── reactor_integration_test.cpp ─────────────────────────────
namespace Reactor {
constexpr std::uint16_t HAPPY_PATH_PORT = 19883;
constexpr std::uint16_t INVALID_FIRST_MESSAGE_PORT = 19884;
constexpr std::uint16_t DISCONNECT_PORT = 19885;
constexpr std::uint16_t THREE_PARTICIPANT_PORT = 19886;

}  // namespace Reactor

// ── epoller_test.cpp ─────────────────────────────────────────
namespace Epoller {
constexpr std::uint16_t ADD_PORT = 19886;
constexpr std::uint16_t REMOVE_PORT = 19887;
constexpr std::uint16_t TIMEOUT_PORT = 19888;
constexpr std::uint16_t READABLE_PORT = 19889;
constexpr std::uint16_t CONTINUE_PORT_1 = 19890;
constexpr std::uint16_t CONTINUE_PORT_2 = 19891;
}  // namespace Epoller

// ── socket_integration_test.cpp ──────────────────────────────
namespace Socket {
constexpr std::uint16_t ECHO_PORT = 9876;     // basic socket echo test
constexpr std::uint16_t UNUSED_PORT = 19999;  // nothing listens here
}  // namespace Socket

// ── tls_socket_integration_test.cpp ──────────────────────────
namespace Tls {
constexpr std::uint16_t ECHO_PORT = 19893;
constexpr std::uint16_t PLAIN_SERVER_PORT = 19894;
constexpr std::uint16_t PLAIN_CLIENT_PORT = 19895;

}  // namespace Tls

// ── agent_integration_test.cpp ───────────────────────────────
namespace Agent {
constexpr std::uint16_t INTEGRATION_PORT = 19892;

}  // namespace Agent

// ── server_integration_test.cpp ──────────────────────────────
namespace Server {
constexpr std::uint16_t INTEGRATION_PORT = 19882;

}  // namespace Server
}  // namespace Ports

#endif