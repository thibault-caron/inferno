#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "agent_session.hpp"
#include "protocol/lptf_protocol.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/SocketFactory.hpp"

namespace {

constexpr std::size_t kReceiveChunkSize = 4096;
constexpr std::chrono::seconds kRetryDelay{10};

const char* messageTypeToString(const MessageType type) {
  switch (type) {
    case MessageType::REGISTER:
      return "REGISTER";
    case MessageType::DATA:
      return "DATA";
    case MessageType::COMMAND:
      return "COMMAND";
    case MessageType::RESPONSE:
      return "RESPONSE";
    case MessageType::DISCONNECT:
      return "DISCONNECT";
    case MessageType::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

std::uint16_t resolveServerPort() {
  const char* portEnv = std::getenv("SERVER_PORT");
  if (!portEnv) {
    std::cout << "SERVER_PORT not found in .env" << SERVER_PORT << '\n';
    return SERVER_PORT;
  }

  try {
    const int parsed = std::stoi(portEnv);
    if (parsed > 0 && parsed <= 65535) {
      std::cout << "Resolved server port from environment: " << parsed << '\n';
      return static_cast<std::uint16_t>(parsed);
    }
  } catch (...) {
  }
  std::cout << "Invalid SERVER_PORT value in .env: " << portEnv
            << ", using default " << SERVER_PORT << '\n';
  return SERVER_PORT;
}

std::string resolveServerHost() {
  const char* hostEnv = std::getenv("SERVER_HOST");
  if (hostEnv && hostEnv[0] != '\0') {
    std::cout << "Resolved server host from environment: " << hostEnv << '\n';
    return std::string(hostEnv);
  }
  return "server";
}

bool sendRaw(AgentSession& session, MessageType type,
             const std::vector<std::uint8_t>& payload = {}) {
  if (payload.size() > MAX_VALUE_INT16) {
    std::cerr << "payload too large\n";
    return false;
  }

  const LptfHeader header{{'L', 'P', 'T', 'F'},
                          LPTF_VERSION,
                          type,
                          static_cast<std::uint16_t>(payload.size())};

  std::cout << "[agent] sending " << messageTypeToString(type)
            << " header+payload bytes=" << (LPTF_HEADER_SIZE + payload.size())
            << "\n";

  const std::vector<std::uint8_t> headerBytes =
      ProtocolSerializer::serializeHeader(header);
  const SocketResult headerResult = session.socket->send(headerBytes);
  if (!headerResult.ok() ||
      static_cast<std::size_t>(headerResult.bytesTransferred) !=
          headerBytes.size()) {
    std::cerr << "[agent] send header failed type=" << messageTypeToString(type)
              << " sent=" << headerResult.bytesTransferred
              << " expected=" << headerBytes.size()
              << " status=" << static_cast<int>(headerResult.error) << "\n";
    return false;
  }

  if (!payload.empty()) {
    const SocketResult payloadResult = session.socket->send(payload);
    if (!payloadResult.ok() ||
        static_cast<std::size_t>(payloadResult.bytesTransferred) !=
            payload.size()) {
      std::cerr << "[agent] send payload failed type="
                << messageTypeToString(type)
                << " sent=" << payloadResult.bytesTransferred
                << " expected=" << payload.size()
                << " status=" << static_cast<int>(payloadResult.error) << "\n";
      return false;
    }
  }

  std::cout << "[agent] send ok type=" << messageTypeToString(type) << "\n";

  return true;
}

bool sendRegister(AgentSession& session) {
  RegisterPayload payload;
  payload.os_type = OSType::LINUX;
  payload.arch = ArchType::X64;
  payload.hostname = "inferno-agent";

  const std::vector<std::uint8_t> registerPayload =
      ProtocolSerializer::serializeRegisterPayload(payload);
  return sendRaw(session, MessageType::REGISTER, registerPayload);
}

bool sendResponse(AgentSession& session, std::uint16_t id,
                  ResponseStatus status, const std::string& data) {
  ResponsePayload payload;
  payload.id = id;
  payload.status = status;
  payload.total_chunks = 1;
  payload.chunk_index = 0;
  payload.data = data;

  const std::vector<std::uint8_t> responsePayload =
      ProtocolSerializer::serializeResponsePayload(payload);
  return sendRaw(session, MessageType::RESPONSE, responsePayload);
}

bool sendProtocolError(AgentSession& session, ErrorType code,
                       const std::string& msg) {
  ErrorPayload payload;
  payload.code = code;
  payload.message = msg;

  const std::vector<std::uint8_t> errorPayload =
      ProtocolSerializer::serializeErrorPayload(payload);
  return sendRaw(session, MessageType::ERROR, errorPayload);
}

SocketResult receiveIntoBuffer(AgentSession& session) {
  std::vector<std::uint8_t> temp(kReceiveChunkSize);
  const SocketResult result = session.socket->recv(temp.data(), temp.size());

  if (result.ok() && result.bytesTransferred > 0) {
    temp.resize(static_cast<std::size_t>(result.bytesTransferred));
    session.buffer.insert(session.buffer.end(), temp.begin(), temp.end());
    std::cout << "[agent] recv bytes=" << result.bytesTransferred
              << " buffer_size=" << session.buffer.size() << "\n";
  } else {
    std::cout << "[agent] recv status=" << static_cast<int>(result.error)
              << " bytes=" << result.bytesTransferred << "\n";
  }

  return result;
}

bool handleFrame(AgentSession& session, const Frame& frame) {
  switch (frame.header.type) {
    case MessageType::COMMAND: {
      try {
        const CommandPayload cmd =
            ProtocolParser::parseCommandPayload(frame.payload);

        if (cmd.type == CommandType::OS_INFO) {
          std::cout << "[agent] received COMMAND OS_INFO id=" << cmd.id << "\n";
          return sendResponse(session, cmd.id, ResponseStatus::OK,
                              "hello world from agent");
        }

        return sendProtocolError(session, ErrorType::UNKNOWN_COMMAND,
                                 "Command not implemented in minimal agent");
      } catch (const std::exception& ex) {
        std::cerr << "[agent] invalid COMMAND payload: " << ex.what() << "\n";
        return sendProtocolError(session, ErrorType::INVALID_FORMAT,
                                 "Invalid COMMAND payload");
      }
    }
    case MessageType::DISCONNECT:
      std::cout << "[agent] received DISCONNECT\n";
      return false;
    case MessageType::ERROR: {
      try {
        const ErrorPayload payload =
            ProtocolParser::parseErrorPayload(frame.payload);
        std::cerr << "[agent] server ERROR code="
                  << static_cast<int>(payload.code)
                  << " message=" << payload.message << "\n";
      } catch (...) {
        std::cerr << "[agent] received malformed ERROR payload\n";
      }
      return true;
    }
    default:
      return sendProtocolError(session, ErrorType::UNKNOWN_TYPE,
                               "Unexpected message type for agent");
  }
}

void runAgentLoop(const std::string& host, std::uint16_t port) {
  bool running = true;
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

    if (!sendRegister(session)) {
      std::cerr << "[agent] failed to send REGISTER, reconnecting...\n";
      session.socket->close();
      ::sleep(static_cast<unsigned>(kRetryDelay.count()));
      continue;
    }
    std::cout << "[agent] REGISTER sent\n";
    // check this loop's condition
    bool connected = true;
    while (connected && session.isValid()) {
      const SocketResult result = receiveIntoBuffer(session);
      if (!result.ok() || result.bytesTransferred <= 0) {
        std::cout << "[agent] connection loop break status="
                  << static_cast<int>(result.error)
                  << " bytes=" << result.bytesTransferred << "\n";
        connected = false;
        break;
      }

      while (std::optional<Frame> frame = session.tryExtractFrame()) {
        std::cout << "[agent] extracted frame type="
                  << messageTypeToString(frame->header.type)
                  << " payload_size=" << frame->payload.size() << "\n";
        // handleframe = dispatcher.dispatch(session, frame.value())
        connected = handleFrame(session, frame.value());
        // throw X
        // if (!connected) {
        //   break;
        // }
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
  const std::string host = resolveServerHost();
  const std::uint16_t port = resolveServerPort();
  // runAgentLoop("127.0.0.1", SERVER_PORT);
  runAgentLoop(host, port);
  return 0;
}

// est-ce que la socket server est déclarée coté client (si oui shared
// dispatcher)