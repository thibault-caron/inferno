#include "socket/tls_socket.hpp"
#include <iostream>
TLSSocket::TLSSocket(std::unique_ptr<ISocket> raw,
                     std::unique_ptr<SSL, SSLDeleter> ssl)
    : raw_(std::move(raw)), ssl_(std::move(ssl)) {}

TLSSocket::TLSSocket(std::unique_ptr<ISocket> raw,
                     std::unique_ptr<SSL_CTX, SSLContextDeleter> context)
    : raw_(std::move(raw)), context_(std::move(context)) {}

bool TLSSocket::connect(const std::string& host, uint16_t port) {
  std::cout << "[DEBUG] Connecting to " << host << ":" << port << std::endl;
  if (!raw_->connect(host, port)) {
    std::cout << "[DEBUG] TCP connect failed" << std::endl;
    return false;
  }

  ssl_ =
      std::unique_ptr<SSL, SSLDeleter>(SSL_new(context_.get()), SSLDeleter{});
  setupSSLSocket(ssl_.get(), raw_->getFd());
  std::cout << "[DEBUG] Created SSL object" << std::endl;
  return SSL_connect(ssl_.get()) == 1;
}

std::unique_ptr<ISocket> TLSSocket::accept() {
  std::unique_ptr<ISocket> client = raw_->accept();

  if (!client) {
    return nullptr;
  }

  std::unique_ptr<SSL, SSLDeleter> ssl =
      std::unique_ptr<SSL, SSLDeleter>(SSL_new(context_.get()), SSLDeleter{});

  setupSSLSocket(ssl.get(), client->getFd());

  if (SSL_accept(ssl.get()) != 1) {
    return nullptr;
  }

  return std::make_unique<TLSSocket>(std::move(client), std::move(ssl));
}

SocketResult TLSSocket::send(const std::uint8_t* data, size_t len) {
  int written = SSL_write(ssl_.get(), data, static_cast<int>(len));

  if (written <= 0) {
    return {-1, translateStatus(SSL_get_error(ssl_.get(), written))};
  }

  return {written, SocketStatus::OK};
}

SocketResult TLSSocket::recv(std::uint8_t* data, std::size_t len) {
  int read = SSL_read(ssl_.get(), data, static_cast<int>(len));

  if (read <= 0) {
    return {-1, translateStatus(SSL_get_error(ssl_.get(), read))};
  }

  return {read, SocketStatus::OK};
}

void TLSSocket::close() {
  if (ssl_) {
    // close
    SSL_shutdown(ssl_.get());
    ssl_.reset();
  }
  raw_->close();
}

SocketStatus TLSSocket::translateStatus(int err) const {
  switch (err) {
    case SSL_ERROR_ZERO_RETURN:
      return SocketStatus::CONNECTION_RESET;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      return SocketStatus::WOULD_BLOCK;
    case SSL_ERROR_SYSCALL:
      return raw_->translateStatus(errno);  // fall back to OS errno
    default:
      return SocketStatus::UNKNOWN;
  }
}

// In tls_socket.cpp, add this helper:

void TLSSocket::setupSSLSocket(SSL* ssl, int fd) {
#ifdef _WIN32
  // Windows: use BIO for socket I/O
  BIO* bio = BIO_new_socket(fd, BIO_NOCLOSE);
  SSL_set_bio(ssl, bio, bio);
#else
  // Unix: use direct file descriptor
   SSL_set_fd(ssl, fd);
#endif
}