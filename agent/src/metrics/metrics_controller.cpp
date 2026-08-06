#include "metrics/metrics_controller.hpp"

#include "codec/metrics_serializer.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"
#include "metrics/metrics_scrapper_factory.hpp"

// normal construction — creates its own scrapper
MetricsController::MetricsController(int intervalMs)
    : intervalMs_(intervalMs), scrapper_(MetricsScrapperFactory::create()) {}

// test construction — scrapper injected
MetricsController::MetricsController(int intervalMs,
                                     std::unique_ptr<IMetricsScrapper> scrapper)
    : intervalMs_(intervalMs), scrapper_(std::move(scrapper)) {}

void MetricsController::tick(AgentSession& session) {
  MetricsSample sample = scrapper_->sample();

  DataPayload data;
  data.subtype = DataType::METRICS_SAMPLE;
  data.data = MetricsSerializer::serializeMetricsSample(sample);

  std::vector<std::uint8_t> payload =
      ProtocolSerializer::serializeDataPayload(data);

  Frame frame = {ProtocolHelper::createHeader(MessageType::DATA, payload),
                 payload};

  session.sendFrame(frame);

  lastSample_ = std::chrono::steady_clock::now();
}

void MetricsController::start(AgentSession& session) {
  active_ = true;
  tick(session);  // immediate first sample, even if zeros
}

void MetricsController::stop() { active_ = false; }

bool MetricsController::isActive() const { return active_; }

bool MetricsController::isDue() const { return msUntilNextSample() == 0; }

int MetricsController::msUntilNextSample() const {
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - lastSample_)
                           .count();
  const int remaining = intervalMs_ - static_cast<int>(elapsed);
  return std::max(0, remaining);
}