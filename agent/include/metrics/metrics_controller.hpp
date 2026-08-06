#ifndef METRICS_CONTROLLER_HPP
#define METRICS_CONTROLLER_HPP

#include <chrono>

#include "agent_session.hpp"
#include "metrics/i_metrics_scrapper.hpp"

class MetricsController {
 public:
  explicit MetricsController(int intervalMs);
  MetricsController(int intervalMs,
                    std::unique_ptr<IMetricsScrapper> scrapper);  // tests

  void start(AgentSession& session);  // isActive = true + immediate first tick
  void stop();                        // isActive = false

  bool isActive() const;
  bool isDue() const;
  int msUntilNextSample() const;

  void tick(AgentSession& session);  // sample → serialize → send DATA frame

 private:
  int intervalMs_;
  bool active_{false};
  std::unique_ptr<IMetricsScrapper> scrapper_;
  std::chrono::steady_clock::time_point lastSample_;
};

#endif