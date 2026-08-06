#include <openssl/err.h>
#include <openssl/ssl.h>

#include "socket/i_socket.hpp"

struct SSLDeleter {
  void operator()(SSL* ptr) const { SSL_free(ptr); }
};

struct SSLContextDeleter {
  void operator()(SSL_CTX* ptr) const { SSL_CTX_free(ptr); }
};

class TLSSocket : public ISocket {
 public:
  // Server side: wraps an already-accepted raw socket + already-created SSL*
  TLSSocket(std::unique_ptr<ISocket> raw, std::unique_ptr<SSL, SSLDeleter> ssl);

  // Agent side: wraps a raw socket, creates SSL from ctx, connects + handshakes
  TLSSocket(std::unique_ptr<ISocket> raw,
            std::unique_ptr<SSL_CTX, SSLContextDeleter> context);

  ~TLSSocket() override { close(); }

  // These have real TLS logic:
  bool connect(const std::string& host, uint16_t port) override;
  std::unique_ptr<ISocket> accept() override;
  SocketResult send(const std::uint8_t* data, size_t len) override;
  SocketResult recv(std::uint8_t* data, size_t len) override;
  void close() override;

  // These just delegate to raw_:
  bool bind(uint16_t port) override { return raw_->bind(port); }
  bool listen(int backlog) override { return raw_->listen(backlog); }
  bool setNonBlocking(bool on) override { return raw_->setNonBlocking(on); }
  bool isValid() const override { return raw_->isValid(); }
  std::string remoteAddress() const override { return raw_->remoteAddress(); }
  uint16_t remotePort() const override { return raw_->remotePort(); }
  int getFd() override { return raw_->getFd(); }
  SocketStatus translateStatus(int err) const override;

 private:
  std::unique_ptr<ISocket> raw_;
  std::unique_ptr<SSL, SSLDeleter> ssl_;
  std::unique_ptr<SSL_CTX, SSLContextDeleter> context_;
  static void setupSSLSocket(SSL* ssl, int fd);
};