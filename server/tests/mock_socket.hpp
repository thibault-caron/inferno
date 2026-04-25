#ifndef MOCK_SOCKET_HPP
#define MOCK_SOCKET_HPP

#include <gmock/gmock.h>

#include "socket/ISocket.hpp"

class MockSocket : public ISocket {
 public:
  MOCK_METHOD(bool, connect, (const std::string&, std::uint16_t), (override));
  MOCK_METHOD(bool, bind, (std::uint16_t), (override));
  MOCK_METHOD(bool, listen, (int), (override));
  MOCK_METHOD(std::unique_ptr<ISocket>, accept, (), (override));
  MOCK_METHOD(SocketResult, send, (const std::uint8_t*, std::size_t),
              (override));
  MOCK_METHOD(SocketResult, recv, (std::uint8_t*, std::size_t), (override));
  MOCK_METHOD(void, close, (), (override));
  MOCK_METHOD(bool, setNonBlocking, (bool), (override));
  MOCK_METHOD(bool, isValid, (), (const, override));
  MOCK_METHOD(int, getFd, (), (override));
  MOCK_METHOD(std::string, remoteAddress, (), (const, override));
  MOCK_METHOD(std::uint16_t, remotePort, (), (const, override));
};

// Helper: make a SocketResult that delivers exactly `data` bytes
// into the output buffer on call, then returns OK.
// Usage:
//   EXPECT_CALL(*sock, recv(_, _))
//       .WillOnce(RecvData(bytes));
ACTION_P(RecvData, bytes) {
  const std::size_t n = std::min(bytes.size(), arg1);
  std::copy(bytes.begin(), bytes.begin() + n, arg0);
  return SocketResult{static_cast<int>(n), SocketStatus::OK};
}

// Helper: pretend the socket wrote the whole requested buffer.
// Usage:
//   EXPECT_CALL(*sock, send(_, _))
//       .WillByDefault(ReturnAllBytes());
ACTION(ReturnAllBytes) {
  return SocketResult{static_cast<int>(arg1), SocketStatus::OK};
}

#endif
