#include "agent_dispatcher.hpp"

AgentDispatcher::AgentDispatcher() = default;

void AgentDispatcher::handleFrame(AgentSession& session, const Frame& frame) {
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

        return sendError(session, ErrorType::UNKNOWN_COMMAND,
                                 "Command not implemented in minimal agent");
      } catch (const std::exception& ex) {
        std::cerr << "[agent] invalid COMMAND payload: " << ex.what() << "\n";
        return sendError(session, ErrorType::INVALID_FORMAT,
                                 "Invalid COMMAND payload");
      }
    }
    // TODO : how agent handle disconnect command from server
    case MessageType::DISCONNECT:
      std::cout << "[agent] received DISCONNECT\n";
      if (session.socket) {
        session.socket->close();
        session.socket.reset();
      }
      session.setRegistered(false);
      return;
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
    }
    default:
      return sendError(session, ErrorType::UNKNOWN_TYPE,
                               "Unexpected message type for agent");
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
      SocketHelper::createHeader(MessageType::REGISTER, registerPayload),
      registerPayload};

  sendFrame(session, frame, senderName);
  std::cout << "at the end of sendRegister\n";
  registerWasSent = true;
}

void AgentDispatcher::sendResponse(AgentSession& session, std::uint16_t id,
                  ResponseStatus status, const std::string& data) {
  ResponsePayload payload;
  payload.id = id;
  payload.status = status;
  payload.total_chunks = 1;
  payload.chunk_index = 0;
  payload.data = data;

  const std::vector<std::uint8_t> responsePayload =
      ProtocolSerializer::serializeResponsePayload(payload);

      Frame frame = {
        SocketHelper::createHeader(MessageType::RESPONSE, responsePayload),
        responsePayload
      };
      sendFrame(session, frame, senderName);
}
