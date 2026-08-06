
#include "metrics/metrics_controller.hpp"

#include <gtest/gtest.h>

#include "agent_session.hpp"
#include "builders/metrics_controller_test_factory.hpp"
#include "protocol/lptf_protocol.hpp"
#include "stubs/fake_metrics_scrapper.hpp"
#include "stubs/spy_socket.hpp"

class MetricsControllerTest : public ::testing::Test {
 public:
  MetricsControllerTest()
      : session(AgentSession(spy.makeUnique())),
        controller(MetricsControllerTestFactory::make(scrapperPtr)) {}

 protected:
  SpySocket spy;
  AgentSession session;
  FakeMetricsScrapper* scrapperPtr = nullptr;
  MetricsController controller;
};

TEST_F(MetricsControllerTest, IsNotActiveByDefault) {
  EXPECT_FALSE(controller.isActive());
}

TEST_F(MetricsControllerTest, StartMakesControllerActive) {
  controller.start(session);
  EXPECT_TRUE(controller.isActive());
}

TEST_F(MetricsControllerTest, StopMakesControllerInactive) {
  controller.start(session);
  EXPECT_TRUE(controller.isActive());
  controller.stop();
  EXPECT_FALSE(controller.isActive());
}

TEST_F(MetricsControllerTest, StartSendsImmediateDataFrame) {
  controller.start(session);
  EXPECT_EQ(spy.messageType(), MessageType::DATA);
  EXPECT_EQ(scrapperPtr->callCount, 1);
}

TEST_F(MetricsControllerTest, IsNotDueImmediatelyAfterTick) {
  controller.start(session);  // tick happens inside
  EXPECT_FALSE(controller.isDue());
}

TEST_F(MetricsControllerTest, IsDueAfterIntervalElapses) {
  auto scrapper = make_unique<FakeMetricsScrapper>();
  MetricsController shortController(1ms, move(scrapper));

  shortController.start(session);
  sleep(5ms);
  EXPECT_TRUE(shortController.isDue());
}
