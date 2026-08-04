#include "socket/windows_socket.hpp"

#include <mutex>

// Winsock must be initialised once per process, before any socket call,
// and released when the last socket is destroyed.
static int g_instanceCount = 0;
static std::mutex g_winsockMutex;

// static int g_instanceCount = 0;

static void initWinsock() {
  std::lock_guard<std::mutex> lock(g_winsockMutex);
  if (g_instanceCount++ == 0) {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
  }
}

static void cleanupWinsock() {
  std::lock_guard<std::mutex> lock(g_winsockMutex);
  if (--g_instanceCount == 0) WSACleanup();
}

WindowsSocket::WindowsSocket() { initWinsock(); }

WindowsSocket::WindowsSocket(SOCKET sock) : socket_(sock) { initWinsock(); }

WindowsSocket::~WindowsSocket() {
  close();
  cleanupWinsock();
}

void WindowsSocket::close() {
  if (socket_ != INVALID_SOCKET) {
    closesocket(socket_);
    socket_ = INVALID_SOCKET;
  }
}

bool WindowsSocket::isValid() const { return socket_ != INVALID_SOCKET; }

// SOCKET is wider than int on 64-bit Windows, but ISocket requires an int.
int WindowsSocket::getFd() { return static_cast<int>(socket_); }

// ===== Server-side methods: unused, the dashboard is a client only =====

bool WindowsSocket::connect(const std::string& host, uint16_t port) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* result = nullptr;
  const std::string portStr = std::to_string(port);
  if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0)
    return false;

  socket_ =
      ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (socket_ == INVALID_SOCKET) {
    freeaddrinfo(result);
    return false;
  }

  const bool ok =
      ::connect(socket_, result->ai_addr,
                static_cast<int>(result->ai_addrlen)) != SOCKET_ERROR;
  freeaddrinfo(result);

  if (!ok) {
    close();
    return false;
  }
  return true;
}

bool WindowsSocket::bind(uint16_t) { return false; }
bool WindowsSocket::listen(int) { return false; }
std::unique_ptr<ISocket> WindowsSocket::accept() { return nullptr; }

SocketResult WindowsSocket::send(const uint8_t* data, size_t len) {
  const int sent = ::send(socket_, reinterpret_cast<const char*>(data),
                          static_cast<int>(len), 0);
  if (sent == SOCKET_ERROR) return {-1, translateStatus(WSAGetLastError())};

  return {sent, SocketStatus::OK};
}

SocketResult WindowsSocket::recv(uint8_t* data, size_t len) {
  const int received =
      ::recv(socket_, reinterpret_cast<char*>(data), static_cast<int>(len), 0);
  if (received == SOCKET_ERROR) return {-1, translateStatus(WSAGetLastError())};

  return {received, SocketStatus::OK};
}

bool WindowsSocket::setNonBlocking(bool on) {
  u_long mode = on ? 1 : 0;
  return ioctlsocket(socket_, FIONBIO, &mode) != SOCKET_ERROR;
}

std::string WindowsSocket::remoteAddress() const {
  sockaddr_in addr{};
  int len = sizeof(addr);
  if (getpeername(socket_, reinterpret_cast<sockaddr*>(&addr), &len) ==
      SOCKET_ERROR)
    return {};

  char buffer[INET_ADDRSTRLEN] = {};
  inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer));
  return buffer;
}

uint16_t WindowsSocket::remotePort() const {
  sockaddr_in addr{};
  int len = sizeof(addr);
  if (getpeername(socket_, reinterpret_cast<sockaddr*>(&addr), &len) ==
      SOCKET_ERROR)
    return 0;

  return ntohs(addr.sin_port);
}

SocketStatus WindowsSocket::translateStatus(int err) const {
  switch (err) {
    case WSAECONNREFUSED:
      return SocketStatus::CONNECTION_REFUSED;
    case WSAECONNRESET:
      return SocketStatus::CONNECTION_RESET;
    case WSAETIMEDOUT:
      return SocketStatus::TIMED_OUT;
    case WSAEWOULDBLOCK:
      return SocketStatus::WOULD_BLOCK;
    case WSAEADDRINUSE:
      return SocketStatus::ADDRESS_IN_USE;
    case WSAEINVAL:
      return SocketStatus::INVALID_ARGUMENT;
    case WSAENETUNREACH:
      return SocketStatus::NETWORK_UNREACHABLE;
    default:
      return SocketStatus::UNKNOWN;
  }
}