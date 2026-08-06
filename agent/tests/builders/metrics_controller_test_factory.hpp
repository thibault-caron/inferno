#ifndef TEST_METRICS_CONTROLLER_FACTORY_HPP
#define TEST_METRICS_CONTROLLER_FACTORY_HPP

#include <memory>
#include "metrics/metrics_controller.hpp"
#include "stubs/fake_metrics_scrapper.hpp"

namespace MetricsControllerTestFactory {
inline std::shared_ptr<MetricsController> make(FakeMetricsScrapper*& outPtr) {
  auto scrapper = std::make_unique<FakeMetricsScrapper>();
  outPtr = scrapper.get();
  return std::make_shared<MetricsController>(100, std::move(scrapper));
}
}  // namespace MetricsControllerTestFactory

#endif