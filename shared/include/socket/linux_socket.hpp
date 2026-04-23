#ifndef LINUX_SOCKET
#define LINUX_SOCKET

#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <fcntl.h>      // fcntl(), O_NONBLOCK
#include <netdb.h>      // getaddrinfo
#include <netinet/in.h>
#include <netinet/tcp.h>  // TCP_NODELAY
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>  // close()

#include <cerrno>
#include <cstring>  // strerror

#include "convert_endian.hpp"
#include "socket/ISocket.hpp"

class LinuxSocket : public ISocket {
 public:
  // Create a fresh socket (client or server use)
  LinuxSocket();

  // Wrap an already-accepted fd (used internally by accept())
  explicit LinuxSocket(int fileDescriptor);

  ~LinuxSocket() override;

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
  int getFd() override { return socketFileDescriptor; }

 private:
  int socketFileDescriptor = -1; // naming convention m_fd for member file descriptor

  // Translates errno → your SocketError
  static SocketStatus translateStatus(int err);
};

#endif