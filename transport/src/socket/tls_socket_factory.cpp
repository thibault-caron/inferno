#include "socket/tls_socket_factory.hpp"

#include <fstream>

#include "socket/socket_factory.hpp"
#include "socket/tls_socket.hpp"

constexpr char* PROD_PATH_TLS_CERT = "/usr/share/inferno/ca.crt";
constexpr char* DEV_PATH_TLS_CERT = "./certs/ca.crt";

std::unique_ptr<ISocket> TLSSocketFactory::createServer(
    const std::string& cert, const std::string& key) {
  // Step 1 — create the SSL_CTX, immediately wrap in unique_ptr
  std::unique_ptr<SSL_CTX, SSLContextDeleter> context(
      SSL_CTX_new(TLS_server_method()),
      SSLContextDeleter{});  // create context, server role

  // Step 2 — configure it
  SSL_CTX_use_certificate_file(context.get(), cert.c_str(),
                               SSL_FILETYPE_PEM);  // load server cert
  SSL_CTX_use_PrivateKey_file(context.get(), key.c_str(),
                              SSL_FILETYPE_PEM);  // load server key
  SSL_CTX_set_verify(context.get(), SSL_VERIFY_NONE,
                     nullptr);  // don't verify clients

  // Step 3 — create raw TCP socket
  auto raw = SocketFactory::createTCP();

  // Step 4 — wrap both into TLSSocket
  return std::make_unique<TLSSocket>(std::move(raw), std::move(context));
}

std::unique_ptr<ISocket> TLSSocketFactory::createClient(
    const std::string& caFile) {
  std::unique_ptr<SSL_CTX, SSLContextDeleter> context(
      SSL_CTX_new(TLS_client_method()), SSLContextDeleter{});

  // Step 2 — configure it

  SSL_CTX_load_verify_locations(context.get(), caFile.c_str(),
                                nullptr);  // load CA cert to trust
  SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER,
                     nullptr);  // verify server cert ← critical

  // Step 3 — create raw TCP socket
  auto raw = SocketFactory::createTCP();

  // Step 4 — wrap both into TLSSocket
  return std::make_unique<TLSSocket>(std::move(raw), std::move(context));
}

std::string TLSSocketFactory::findCACertificate() {
  // 1. AppImage path
  const char* appdir = std::getenv("APPDIR");
  if (appdir) {
    std::string appimage_path = std::string(appdir) + PROD_PATH_TLS_CERT;
    std::ifstream cert(appimage_path);
    if (cert.is_open()) {
      cert.close();
      return appimage_path;
    }
  }
  // 2. Dev fallback
  std::ifstream dev_cert(DEV_PATH_TLS_CERT);
  if (dev_cert.is_open()) {
    dev_cert.close();
    return DEV_PATH_TLS_CERT;
  }

  // Fallback - will fail at TLS init if not found
  return DEV_PATH_TLS_CERT;
}