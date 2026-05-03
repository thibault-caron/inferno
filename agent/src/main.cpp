#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "agent_dispatcher.hpp"
#include "agent_session.hpp"
#include "env_helper.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/socket_factory.hpp"
#include "socket/socket_helper.hpp"

namespace {

constexpr std::chrono::seconds kRetryDelay{5};

void runAgentLoop(const std::string& host, std::uint16_t port) {
  bool running = true;
  AgentDispatcher dispatcher;
  // Set running = false on SIGINT via signal handler (not implemented here)
  while (running) {
    std::unique_ptr<ISocket> socket = SocketFactory::createTCP();

    if (!socket || !socket->connect(host, port)) {
      std::cerr << "[agent] connect failed to " << host << ':' << port
                << ", retrying...\n";
      ::sleep(static_cast<unsigned>(kRetryDelay.count()));
      continue;
    }

    std::cout << "[agent] connected to " << host << ':' << port << "\n";
    AgentSession session(std::move(socket));

    dispatcher.sendRegister(session);
    if (!dispatcher.getRegisterWasSent()) {
      std::cerr << "[agent] failed to send REGISTER, reconnecting...\n";
      session.socket->close();
      ::sleep(static_cast<unsigned>(kRetryDelay.count()));
      continue;
    }
    // TODO : double free or corruption after sending register ?
    std::cout << "[agent] REGISTER sent\n";
    // check this loop's condition
    // bool connected = true;
    // while (connected && session.isValid())
    while (session.isValid()) {
      const SocketResult result = SocketHelper::receiveIntoBuffer(session);
      if (!result.ok() || result.bytesTransferred <= 0) {
        std::cout << "[agent] connection loop break status="
                  << static_cast<int>(result.error)
                  << " bytes=" << result.bytesTransferred << "\n";
        session.socket->close();
        // connected = false;
        // break;
      }

      while (std::optional<Frame> frame = session.tryExtractFrame()) {
        std::cout << "[agent] extracted frame type="
                  << SocketHelper::messageTypeToString(frame->header.type)
                  << " payload_size=" << frame->payload.size() << "\n";
        dispatcher.handleFrame(session, frame.value());
      }
    }

    if (session.socket) {
      session.socket->close();
    }

    std::cout << "[agent] disconnected, reconnecting...\n";
    ::sleep(static_cast<unsigned>(kRetryDelay.count()));
  }
}

}  // namespace

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  const std::string host = EnvHelper::resolveServerHost();
  const std::uint16_t port = EnvHelper::resolveServerPort();
  runAgentLoop(host, port);
  return 0;
}