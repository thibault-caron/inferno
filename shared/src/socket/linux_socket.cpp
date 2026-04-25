#include "socket/linux_socket.hpp"
/*
:: with nothing on the left
In C++, :: means "look in the global namespace".
When you write ::socket(), you're saying "call
the socket function that comes from the OS, not
any class or namespace I defined".
It's a safety habit. If you had your own function
also called socket, the compiler would get confused.
::socket() removes all ambiguity: "the OS one, specifically". => Defensive style
*/

LinuxSocket::LinuxSocket() {
  // SOCK_STREAM = TCP. Use SOCK_DGRAM for UDP.
  // AF_INET = IPv4. Use AF_INET6 for IPv6, or handle both.
  socketFileDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);
  // socketFileDescriptor == -1 on failure, isValid() will return false
  /*
  AF_INET = use IPv4 addresses (the classic 192.168.x.x style)
  SOCK_STREAM = use TCP (reliable, ordered, connection-based)
  0 = let the OS pick the right protocol automatically (TCP in this case)
  */

  // Useful for servers: reuse address immediately after restart
  if (socketFileDescriptor != -1) {
    int opt = 1;
    ::setsockopt(socketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt));

    // Disable Nagle's algorithm if your protocol sends small packets
    // ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
  }
}

LinuxSocket::LinuxSocket(int fileDescriptor)
    : socketFileDescriptor(fileDescriptor) {}

LinuxSocket::~LinuxSocket() { close(); }

bool LinuxSocket::connect(const std::string& host, uint16_t port) {
  // Use getaddrinfo() — it handles both hostnames and IP strings,
  // and it supports IPv4 and IPv6.
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;  // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  std::string portString = std::to_string(port);

  int hostnameResolutionStatus = ::getaddrinfo(host.c_str(), portString.c_str(), &hints, &result);
  if (hostnameResolutionStatus != 0) return false;

  bool connected = false;
  for (addrinfo* candidate = result; candidate != nullptr; candidate = candidate->ai_next) {
    if (::connect(socketFileDescriptor, candidate->ai_addr, candidate->ai_addrlen) == 0) {
      connected = true;
      break;
    }
  }
  ::freeaddrinfo(result);
  return connected;
}

bool LinuxSocket::bind(uint16_t port) {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
  addr.sin_port = htons(port);        // htons = host-to-network byte order
                                      // (but you already handle endianness!)
  return ::bind(socketFileDescriptor, reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)) == 0;
}

bool LinuxSocket::listen(int backlog) {
  return ::listen(socketFileDescriptor, backlog) == 0;
}

// blocking call - Each call to accept() gives you a new socket for each new client.
std::unique_ptr<ISocket> LinuxSocket::accept() {
  sockaddr_in clientAddr{};
  socklen_t addrLen = sizeof(clientAddr);

  int clientFd = ::accept(socketFileDescriptor,
                          reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
  if (clientFd == -1) return nullptr;

  return std::make_unique<LinuxSocket>(clientFd);
}

SocketResult LinuxSocket::send(const uint8_t* data, size_t length) {
  // MSG_NOSIGNAL: don't raise SIGPIPE if the peer closed the connection.
  // Without this, a broken pipe kills your process silently on Linux.
  ssize_t sent = ::send(socketFileDescriptor, static_cast<const void*>(data),
                        length, MSG_NOSIGNAL);
  if (sent == -1) {
    return {-1, translateStatus(errno)};
  }
  return {static_cast<int>(sent), SocketStatus::OK};
}

SocketResult LinuxSocket::recv(uint8_t* data, size_t length) {
  ssize_t received =
      ::recv(socketFileDescriptor, static_cast<void*>(data), length, 0);
  if (received == -1) {
    return {-1, translateStatus(errno)};
  }
  if (received == 0) {
    // recv() returning 0 = peer shut down the connection cleanly
    return {0, SocketStatus::CONNECTION_RESET};
  }
  return {static_cast<int>(received), SocketStatus::OK};
}

void LinuxSocket::close() {
  if (socketFileDescriptor != -1) {
    ::close(
        socketFileDescriptor);  // Linux: close() the fd. Windows: closesocket()
    socketFileDescriptor = -1;
  }
}

bool LinuxSocket::setNonBlocking(bool on) {
  int flags = ::fcntl(socketFileDescriptor, F_GETFL, 0);
  if (flags == -1) return false;
  flags = on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return ::fcntl(socketFileDescriptor, F_SETFL, flags) == 0;
}

bool LinuxSocket::isValid() const { return socketFileDescriptor != -1; }

std::string LinuxSocket::remoteAddress() const {
  sockaddr_in address{};
  socklen_t length = sizeof(address);
  if (::getpeername(socketFileDescriptor, reinterpret_cast<sockaddr*>(&address),
                    &length) == -1)
    return "";
  char buffer[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &address.sin_addr, buffer, sizeof(buffer));
  return buffer;
}

uint16_t LinuxSocket::remotePort() const {
  sockaddr_in address{};
  socklen_t length = sizeof(address);
  if (::getpeername(socketFileDescriptor, reinterpret_cast<sockaddr*>(&address),
                    &length) == -1)
    return 0;
  return ntohs(address.sin_port);
}

SocketStatus LinuxSocket::translateStatus(int err) {
  switch (err) {
    case ECONNREFUSED:
      return SocketStatus::CONNECTION_REFUSED;
    case ECONNRESET:
      return SocketStatus::CONNECTION_RESET;
    case ETIMEDOUT:
      return SocketStatus::TIMED_OUT;
    case EAGAIN:  // EAGAIN and EWOULDBLOCK are often the same value,
    #if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
    #endif
      return SocketStatus::WOULD_BLOCK;  // but not always
    case EADDRINUSE:
      return SocketStatus::ADDRESS_IN_USE;
    case EINVAL:
      return SocketStatus::INVALID_ARGUMENT;
    case ENETUNREACH:
      return SocketStatus::NETWORK_UNREACHABLE;
    default:
      return SocketStatus::UNKNOWN;
  }
}