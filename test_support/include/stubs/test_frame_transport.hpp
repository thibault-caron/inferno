#ifndef TEST_FRAME_TRANSPORT_HPP
#define TEST_FRAME_TRANSPORT_HPP

#include "frame_transport.hpp"
#include "spy_socket.hpp"

// At the top of agent_session_test.cpp, inside the test file
class TestFrameTransport : public FrameTransport {
 public:
  using FrameTransport::FrameTransport;
  const OsInfoPayload& getAgentInfo() const override {
    static const OsInfoPayload dummy{};
    return dummy;
  }
  void setAgentInfo([[maybe_unused]] const OsInfoPayload& info) override {}
};

inline TestFrameTransport makeTestFrameTransport(SpySocket& spy) {
  return TestFrameTransport(spy.makeUnique());
}

#endif