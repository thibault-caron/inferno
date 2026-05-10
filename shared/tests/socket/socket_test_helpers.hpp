#ifndef SHARED_TEST_SOCKET_HELPERS_HPP
#define SHARED_TEST_SOCKET_HELPERS_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <vector>

inline bool sendAll(int fd, const std::vector<std::uint8_t>& bytes) {
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const ssize_t sent =
        ::send(fd, bytes.data() + offset, bytes.size() - offset, 0);
    if (sent <= 0) return false;
    offset += static_cast<std::size_t>(sent);
  }
  return true;
}

inline std::vector<std::uint8_t> recvExact(int fd, std::size_t size) {
  std::vector<std::uint8_t> bytes(size);
  std::size_t offset = 0;
  while (offset < size) {
    const ssize_t received =
        ::recv(fd, bytes.data() + offset, size - offset, 0);
    if (received <= 0) {
      bytes.resize(offset);
      return bytes;
    }
    offset += static_cast<std::size_t>(received);
  }
  return bytes;
}

inline int connectLoopback(std::uint16_t port, const char* ip = "127.0.0.1") {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) return -1;

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (::inet_pton(AF_INET, ip, &address.sin_addr) != 1) {
    ::close(fd);
    return -1;
  }

  if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
      0) {
    ::close(fd);
    return -1;
  }

  return fd;
}

inline int startServer(std::uint16_t port, int backlog = 1,
                       bool reuseAddr = true) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) return -1;

  if (reuseAddr) {
    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    ::close(fd);
    return -1;
  }

  if (::listen(fd, backlog) != 0) {
    ::close(fd);
    return -1;
  }

  return fd;
}

#endif
