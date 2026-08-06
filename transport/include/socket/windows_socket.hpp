#ifndef WINDOWS_SOCKET_H
#define WINDOWS_SOCKET_H

// windows.h defines ERROR, min and max as macros, which clash with
// MessageType::INFERNO_ERROR and ResponseStatus::INFERNO_ERROR. These guards
// prevent that.
#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define NOGDI

#include <winsock2.h>
#include <ws2tcpip.h>

#include "socket/i_socket.hpp"

class WindowsSocket : public ISocket {
 public:
  WindowsSocket();
  explicit WindowsSocket(SOCKET sock);
  ~WindowsSocket() override;

  bool connect(const std::string& host, uint16_t port) override;
  bool bind(uint16_t port) override;
  bool listen(int backlog) override;
  std::unique_ptr<ISocket> accept() override;
  SocketResult send(const uint8_t* data, size_t len) override;
  SocketResult recv(uint8_t* data, size_t len) override;
  void close() override;
  bool setNonBlocking(bool on) override;
  bool isValid() const override;
  std::string remoteAddress() const override;
  uint16_t remotePort() const override;
  int getFd() override;
  SocketStatus translateStatus(int err) const override;

 private:
  SOCKET socket_ = INVALID_SOCKET;
};

#endif  // WINDOWS_SOCKET_H
