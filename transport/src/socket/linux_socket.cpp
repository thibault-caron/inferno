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
  socketFileDescriptor_ = ::socket(AF_INET6, SOCK_STREAM, 0);
  // socketFileDescriptor_ == -1 on failure, isValid() will return false
  /*
  AF_INET = use IPv4 addresses (the classic 192.168.x.x style)
  SOCK_STREAM = use TCP (reliable, ordered, connection-based)
  0 = let the OS pick the right protocol automatically (TCP in this case)
  */

  // Useful for servers: reuse address immediately after restart
  if (socketFileDescriptor_ != -1) {
    // Allow both IPv4 and IPv6
    int opt = 0;
    ::setsockopt(socketFileDescriptor_, IPPROTO_IPV6, IPV6_V6ONLY, &opt,
                 sizeof(opt));

    // Reuse adress
    opt = 1;
    ::setsockopt(socketFileDescriptor_, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt));

    // Disable Nagle's algorithm if your protocol sends small packets
    // ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
  }
}

LinuxSocket::LinuxSocket(int fileDescriptor)
    : socketFileDescriptor_(fileDescriptor) {}

LinuxSocket::~LinuxSocket() { close(); }

bool LinuxSocket::connect(const std::string& host, uint16_t port) {
  // Use getaddrinfo() — it handles both hostnames and IP strings,
  // and it supports IPv4 and IPv6.
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;  // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  std::string portString = std::to_string(port);

  int hostnameResolutionStatus =
      ::getaddrinfo(host.c_str(), portString.c_str(), &hints, &result);
  if (hostnameResolutionStatus != 0) return false;

  bool connected = false;
  for (addrinfo* candidate = result; candidate != nullptr;
       candidate = candidate->ai_next) {
     // If socket family doesn't match candidate family, recreate socket
    if (candidate->ai_family != AF_INET6 || 
        (candidate->ai_family == AF_INET && socketFileDescriptor_ == -1)) {
      
      if (socketFileDescriptor_ != -1) {
        ::close(socketFileDescriptor_);
      }
      
      socketFileDescriptor_ = ::socket(candidate->ai_family, SOCK_STREAM, 0);
      if (socketFileDescriptor_ == -1) continue;
      
      // Reapply socket options if we created an IPv6 socket
      if (candidate->ai_family == AF_INET6) {
        int opt = 0;
        ::setsockopt(socketFileDescriptor_, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
        opt = 1;
        ::setsockopt(socketFileDescriptor_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      }
    }
    
    if (::connect(socketFileDescriptor_, candidate->ai_addr,
                  candidate->ai_addrlen) == 0) {
      connected = true;
      break;
    }
  }
  ::freeaddrinfo(result);
  return connected;
}

bool LinuxSocket::bind(uint16_t port) {
  sockaddr_in6 addr{};
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;  //    INADDR_ANY;  // Listen on all interfaces
  addr.sin6_port = htons(port);  // htons = host-to-network byte order
                                 // (but you already handle endianness!)
  return ::bind(socketFileDescriptor_, reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)) == 0;
}

bool LinuxSocket::listen(int backlog) {
  return ::listen(socketFileDescriptor_, backlog) == 0;
}

// blocking call - Each call to accept() gives you a new socket for each new
// agent.
std::unique_ptr<ISocket> LinuxSocket::accept() {
  sockaddr_storage agentAddr{};
  socklen_t addrLen = sizeof(agentAddr);
  // sockaddr_in agentAddr{};
  // socklen_t addrLen = sizeof(agentAddr);

  int agentFd = ::accept(socketFileDescriptor_,
                         reinterpret_cast<sockaddr*>(&agentAddr), &addrLen);
  if (agentFd == -1) return nullptr;

  return std::make_unique<LinuxSocket>(agentFd);
}

SocketResult LinuxSocket::send(const uint8_t* data, size_t length) {
  // MSG_NOSIGNAL: don't raise SIGPIPE if the peer closed the connection.
  // Without this, a broken pipe kills your process silently on Linux.
  ssize_t sent = ::send(socketFileDescriptor_, static_cast<const void*>(data),
                        length, MSG_NOSIGNAL);
  if (sent == -1) {
    return {-1, translateStatus(errno)};
  }
  return {static_cast<int>(sent), SocketStatus::OK};
}

SocketResult LinuxSocket::recv(uint8_t* data, size_t length) {
  ssize_t received =
      ::recv(socketFileDescriptor_, static_cast<void*>(data), length, 0);
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
  if (socketFileDescriptor_ != -1) {
    ::close(socketFileDescriptor_);  // Linux: close() the fd. Windows:
                                     // closesocket()
    socketFileDescriptor_ = -1;
  }
}

bool LinuxSocket::setNonBlocking(bool on) {
  int flags = ::fcntl(socketFileDescriptor_, F_GETFL, 0);
  if (flags == -1) return false;
  flags = on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  return ::fcntl(socketFileDescriptor_, F_SETFL, flags) == 0;
}

bool LinuxSocket::isValid() const { return socketFileDescriptor_ != -1; }

std::string LinuxSocket::remoteAddress() const {
  sockaddr_storage address{};
  socklen_t length = sizeof(address);
  if (::getpeername(socketFileDescriptor_,
                    reinterpret_cast<sockaddr*>(&address), &length) == -1)
    return "";
  char buffer[INET6_ADDRSTRLEN];  // Large enough for IPv6

  if (address.ss_family == AF_INET6) {
    auto* addr6 = reinterpret_cast<sockaddr_in6*>(&address);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buffer, sizeof(buffer));
  } else if (address.ss_family == AF_INET) {
    auto* addr4 = reinterpret_cast<sockaddr_in*>(&address);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buffer, sizeof(buffer));
  }

  return buffer;
}

uint16_t LinuxSocket::remotePort() const {
  sockaddr_storage address{};
  socklen_t length = sizeof(address);
  if (::getpeername(socketFileDescriptor_, reinterpret_cast<sockaddr*>(&address),
                    &length) == -1)
    return 0;

  if (address.ss_family == AF_INET6) {
    return ntohs(reinterpret_cast<sockaddr_in6*>(&address)->sin6_port);
  } else if (address.ss_family == AF_INET) {
    return ntohs(reinterpret_cast<sockaddr_in*>(&address)->sin_port);
  }
  return 0;
}

SocketStatus LinuxSocket::translateStatus(int err) const {
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