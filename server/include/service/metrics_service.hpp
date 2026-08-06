#ifndef METRICS_SERVICE_HPP
#define METRICS_SERVICE_HPP

#include <optional>
#include <string>
#include <vector>

#include "agent_connection.hpp"
#include "protocol/lptf_protocol.hpp"
#include "repository/database_connection.hpp"
#include "repository/metrics_repository.hpp"
#include "session_manager.hpp"

class IMetricsService {
 public:
  virtual ~IMetricsService() = default;
  virtual Frame save(const std::string& agent_id,
                     const MetricsSample& sample) = 0;
};

// agent_repository.hpp
class MetricsService : public IMetricsService {
 private:
  IMetricsRepository& repository_;
  SessionManager& sessionManager_;

 public:
  explicit MetricsService(IMetricsRepository& repository,
                          SessionManager& sessionManager)
      : repository_(repository), sessionManager_(sessionManager) {}

  Frame save(const std::string& agent_id, const MetricsSample& sample) override;
};

#endif