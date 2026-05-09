#ifndef TEST_CONSTANTS_HPP
#define TEST_CONSTANTS_HPP

#include <cstdint>

namespace TestConstants {

// ── tcp_server_test.cpp ──────────────────────────────────────
constexpr std::uint16_t TCP_SERVER_AVAILABLE_PORT = 19872;  // start() succeeds
constexpr std::uint16_t TCP_SERVER_DOUBLE_START_PORT =
    19873;  // start() called twice
constexpr std::uint16_t TCP_SERVER_OCCUPIED_PORT = 19876;  // port already taken
constexpr std::uint16_t TCP_SERVER_NOT_STARTED_PORT =
    19877;  // accept before start
constexpr std::uint16_t TCP_SERVER_AGENT_CONNECT_PORT =
    19879;                                             // single agent connects
constexpr std::uint16_t TCP_SERVER_ECHO_PORT = 19880;  // echo data back
constexpr std::uint16_t TCP_SERVER_REMOTE_ADDR_PORT =
    19881;  // check remote addr/port

// ── server_integration_test.cpp ──────────────────────────────
constexpr std::uint16_t SERVER_INTEGRATION_PORT = 19882;

// ── reactor_integration_test.cpp ─────────────────────────────
constexpr std::uint16_t REACTOR_HAPPY_PATH_PORT = 19883;
constexpr std::uint16_t REACTOR_INVALID_FIRST_MESSAGE_PORT = 19884;
constexpr std::uint16_t REACTOR_DISCONNECT_PORT = 19885;

// ── agent_integration_test.cpp ───────────────────────────────
constexpr std::uint16_t AGENT_INTEGRATION_PORT = 19892;

// ── socket_integration_test.cpp ──────────────────────────────
constexpr std::uint16_t SOCKET_ECHO_PORT = 9876;     // basic socket echo test
constexpr std::uint16_t SOCKET_UNUSED_PORT = 19999;  // nothing listens here

}  // namespace TestConstants

#endif