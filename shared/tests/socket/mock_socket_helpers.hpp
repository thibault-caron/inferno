#ifndef SHARED_TEST_MOCK_SOCKET_HELPERS_HPP
#define SHARED_TEST_MOCK_SOCKET_HELPERS_HPP

#include <gmock/gmock.h>

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "agent_session.hpp"
#include "socket/mock_socket.hpp"

inline std::function<SocketResult(const std::uint8_t*, std::size_t)>
CaptureSentBytes(std::vector<std::uint8_t>& destination) {
  return [&destination](const std::uint8_t* data,
                        std::size_t byteCount) -> SocketResult {
    destination.assign(data, data + byteCount);
    return SocketResult{static_cast<int>(byteCount), SocketStatus::OK};
  };
}

inline std::pair<AgentSession, std::reference_wrapper<MockSocket>>
makeAgentSessionWithMock() {
  std::unique_ptr<MockSocket> mock = std::make_unique<MockSocket>();
  MockSocket& mockRef = *mock;
  AgentSession session(std::move(mock));

  ON_CALL(mockRef, isValid()).WillByDefault(::testing::Return(true));
  ON_CALL(mockRef, send(::testing::_, ::testing::_))
      .WillByDefault(ReturnAllBytes());

  return {std::move(session), std::ref(mockRef)};
}

#endif
