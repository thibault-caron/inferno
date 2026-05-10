#include "agent_dispatcher.hpp"

#include <sstream>

#include "logger.hpp"
#include "protocol/protocol_parser.hpp"
#include "protocol/protocol_serializer.hpp"
#include "socket/i_socket.hpp"
#include "protocol/protocol_helper.hpp"

AgentDispatcher::AgentDispatcher() : Dispatcher("agent") {}

void AgentDispatcher::handleFrame(AgentSession& session, const Frame& frame) {
  switch (frame.header.type) {
    case MessageType::COMMAND:
      return onCommand(session, frame.payload);
    case MessageType::DISCONNECT:
      return onDisconnect(session);
    case MessageType::ERROR: {
      return onError(frame.payload);
    }
    default:
      return sendError(session, ErrorType::UNKNOWN_TYPE,
                       "Unexpected message type for agent");
  }
}
void AgentDispatcher::onError(const std::vector<std::uint8_t>& payload) {
  std::ostringstream what;
  try {
    const ErrorPayload errorPayload =
        ProtocolParser::parseErrorPayload(payload);
    what << "server ERROR code=" << static_cast<int>(errorPayload.code)
         << " message=" << errorPayload.message;
    logger_.error(what.str());
  } catch (...) {
    logger_.error("received malformed ERROR payload");
  }
}
void AgentDispatcher::onDisconnect(AgentSession& session) {
  logger_.info("received DISCONNECT");
  // std::cout << "[agent] received DISCONNECT\n";
  // if (session.isValid()) {
    // session.socket->close();
    session.close(); // check for valid socket already inside close method
    // session.socket.reset();
  // }
  // session.setRegistered(false);
}

void AgentDispatcher::onCommand(AgentSession& session,
                                const std::vector<std::uint8_t>& payload) {
  try {
    const CommandPayload cmd = ProtocolParser::parseCommandPayload(payload);

    if (cmd.type == CommandType::OS_INFO) {
      std::ostringstream what;
      what << "received COMMAND OS_INFO id=" << cmd.id;
      logger_.info(what.str());
      // std::cout << "[agent] received COMMAND OS_INFO id=" << cmd.id <<
      // "\n";
      return sendResponse(session, cmd.id, ResponseStatus::OK,
                          "hello world from agent");
    }

    return sendError(session, ErrorType::UNKNOWN_COMMAND,
                     "Command not implemented in minimal agent");
  } catch (const std::exception& ex) {
    std::ostringstream what;
    what << "invalid COMMAND payload: " << ex.what();
    logger_.error(what.str());
    return sendError(session, ErrorType::INVALID_FORMAT,
                     "Invalid COMMAND payload");
  }
}

void AgentDispatcher::sendRegister(AgentSession& session) {
  RegisterPayload payload;
  payload.os_type = OSType::LINUX;
  payload.arch = ArchType::X64;
  payload.hostname = "inferno-agent";

  const std::vector<std::uint8_t> registerPayload =
      ProtocolSerializer::serializeRegisterPayload(payload);

  Frame frame = {
      ProtocolHelper::createHeader(MessageType::REGISTER, registerPayload),
      registerPayload};

  sendFrame(session, frame);
  session.setAgentInfo(payload);
  session.setRegistered_(RegisterState::SENT);
  std::ostringstream what;
  what << "at the end of sendRegister";
  logger_.info(what.str());
  // session.registered_ = StatusRegister::SENT;
}

void AgentDispatcher::sendResponse(AgentSession& session, std::uint16_t id,
                                   ResponseStatus status,
                                   const std::string& data) {
  ResponsePayload payload;
  payload.id = id;
  payload.status = status;
  payload.total_chunks = 1;
  payload.chunk_index = 0;
  payload.data = data;

  const std::vector<std::uint8_t> responsePayload =
      ProtocolSerializer::serializeResponsePayload(payload);

  Frame frame = {
      ProtocolHelper::createHeader(MessageType::RESPONSE, responsePayload),
      responsePayload};
  sendFrame(session, frame);
}
