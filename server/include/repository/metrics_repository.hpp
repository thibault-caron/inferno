#ifndef METRICS_REPOSITORY_HPP
#define METRICS_REPOSITORY_HPP

#include <optional>
#include <string>
#include <vector>

#include "protocol/types/metrics.hpp"
#include "repository/database_connection.hpp"

class IMetricsRepository {
 public:
  virtual ~IMetricsRepository() = default;

  virtual void save(const std::string& agent_id,
                    const MetricsSample& sample) = 0;

  virtual std::vector<MetricsSample> findByAgent(
      const std::string& agent_id, const std::string& since_iso,
      const std::string& until_iso) = 0;

  virtual std::optional<MetricsSample> getLatest(
      const std::string& agent_id) = 0;
};

class MetricsRepository : public IMetricsRepository {
 private:
  IDatabaseConnection& db_;

  void saveDiskMetric(const std::string& agent_id, const std::string& timestamp,
                      const DiskSample& disk);

  void saveDiskMetrics(const std::string& agent_id,
                       const std::string& timestamp,
                       const std::vector<DiskSample>& disks);
  void saveNetMetric(const std::string& agent_id, const std::string& timestamp,
                     const NetSample& interface);

  void saveNetMetrics(const std::string& agent_id, const std::string& timestamp,
                      const std::vector<NetSample>& interfaces);

  void getDiskSamples(MetricsSample& sample, const std::string& agent_id,
                      const std::string& timestamp);
  void getNetSamples(MetricsSample& sample,
                                        const std::string& agent_id,
                                        const std::string& timestamp);

  MetricsSample reconstructSample(const std::string& agent_id,
                                  const std::string& timestamp);

  std::string serializeCpuPerCore(const std::vector<float>& per_core);
  std::vector<float> deserializeCpuPerCore(const std::string& arrayStr);

 public:
  explicit MetricsRepository(IDatabaseConnection& db) : db_(db) {}

  void save(const std::string& agent_id, const MetricsSample& sample) override;

  std::vector<MetricsSample> findByAgent(const std::string& agent_id,
                                         const std::string& since_iso,
                                         const std::string& until_iso) override;

  std::optional<MetricsSample> getLatest(const std::string& agent_id) override;
};

#endif