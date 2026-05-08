#include "reactor.hpp"

#include <sstream>

void Reactor::run() {
  poller_.add(server_.getFd(), WatchFlags::READ | WatchFlags::ERROR);
  running_ = true;

  std::vector<ReadyEvent> events;

  while (running_) {
    const int firedCount = poller_.wait(events, -1);
    if (firedCount <= 0) {
      running_ = false;
    } else {
      for (const ReadyEvent& event : events) {
        if (event.fileDescriptor == server_.getFd()) {
          onNewConnection();
        } else if (event.readable) {
          onAgentReady(event.fileDescriptor);
        } else if (event.error) {
          onAgentDisconnected(event.fileDescriptor);
        }
      }
    }
  }
}

void Reactor::onNewConnection() {
  std::unique_ptr<ISocket> incoming = server_.acceptAgent();
  if (incoming) {
    const int fd = incoming->getFd();
    if (!poller_.add(fd, WatchFlags::READ | WatchFlags::ERROR)) {
      std::ostringstream what;
      what << "Failed to watch agent fd " << fd;
      logger_.error(what.str());
      return;
    }
    std::ostringstream what;
    what << "Agent connected: " << incoming->remoteAddress() << ':'
         << incoming->remotePort();
    logger_.info(what.str());
    agents_.emplace(fd, AgentSession(std::move(incoming)));
  }
}

void Reactor::onAgentReady(int fileDescriptor) {
  AgentSession& session = agents_.at(fileDescriptor);
  const SocketResult result = session.receiveIntoBuffer();

  if (!result.ok() || result.error == SocketStatus::CONNECTION_RESET ||
      result.bytesTransferred <= 0) {
        std::ostringstream what;
        what << "Agent " << fileDescriptor << " disconnected\n";
        logger_.info(what.str());
    // std::cout << "[server] Agent " << fileDescriptor << " disconnected\n";
    onAgentDisconnected(fileDescriptor);
    return;
  }

  while (std::optional<Frame> frame = session.tryExtractFrame()) {
    if (!session.isRegistered() &&
        frame->header.type != MessageType::REGISTER) {
      dispatcher_.sendError(session, ErrorType::INVALID_FORMAT,
                            "First message must be REGISTER");
    } else {
      dispatcher_.handleFrame(session, frame.value());
    }
  }
}

void Reactor::onAgentDisconnected(int fileDescriptor) {
  poller_.remove(fileDescriptor);
  agents_.erase(fileDescriptor);
}