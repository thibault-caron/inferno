#ifndef FAKE_METRICS_REPOSITORY_HPP
#define FAKE_METRICS_REPOSITORY_HPP

#include <optional>
#include <string>
#include <utility>
#include <vector>


#include "repository/metrics_repository.hpp"
#include "builders/metrics_builder.hpp"

class FakeMetricsRepository : public IMetricsRepository {
 public:
  void save(const std::string& agent_id,
            const MetricsSample& sample) override {
    samples_.push_back({agent_id, sample});
  }

  std::vector<MetricsSample> findByAgent(
      const std::string& agent_id,
      const std::string& /*since_iso*/,
      const std::string& /*until_iso*/) override {

    std::vector<MetricsSample> result;

    for (const auto& entry : samples_) {
      if (entry.agent_id == agent_id) {
        result.push_back(entry.sample);
      }
    }

    // If nothing was saved, just return one fake sample.
    if (result.empty()) {
      result.push_back(MetricsBuilder::createMetricsSample());
    }

    return result;
  }

  std::optional<MetricsSample> getLatest(
      const std::string& agent_id) override {

    for (auto it = samples_.rbegin(); it != samples_.rend(); ++it) {
      if (it->agent_id == agent_id) {
        return it->sample;
      }
    }

    // Always return a fake sample if nothing exists.
    return MetricsBuilder::createMetricsSample();
  }

 private:
  struct Entry {
    std::string agent_id;
    MetricsSample sample;
  };

  std::vector<Entry> samples_;
};

#endif