#include "repository/metrics_repository.hpp"

void MetricsRepository::save(const std::string& agent_id,
                             const MetricsSample& sample) {
  // Insert base (CPU + Memory)
  pqxx::params params;
  params.append(sample.timestamp);
  params.append(agent_id);
  params.append(sample.cpu.total_percent);
  params.append(static_cast<int>(sample.cpu.per_core.size()));
  params.append(serializeCpuPerCore(sample.cpu.per_core));
  params.append(sample.mem.phys_total);
  params.append(sample.mem.phys_used);
  params.append(sample.mem.phys_available);
  params.append(sample.mem.swap_total);
  params.append(sample.mem.swap_used);

  db_.executeParams(
      "INSERT INTO metrics "
      "  (time, agent_id, cpu_total_percent, cpu_core_count, cpu_per_core, "
      "   mem_phys_total, mem_phys_used, mem_phys_available, "
      "   mem_swap_total, mem_swap_used) "
      "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)",
      params);

  saveDiskMetrics(agent_id, sample.timestamp, sample.disks);
  saveNetMetrics(agent_id, sample.timestamp, sample.interfaces);
}

void MetricsRepository::saveDiskMetrics(const std::string& agent_id,
                                        const std::string& timestamp,
                                        const std::vector<DiskSample>& disks) {
  for (const auto& disk : disks) {
    saveDiskMetric(agent_id, timestamp, disk);
  }
}

void MetricsRepository::saveDiskMetric(const std::string& agent_id,
                                       const std::string& timestamp,
                                       const DiskSample& disk) {
  pqxx::params params;
  params.append(timestamp);
  params.append(agent_id);
  params.append(disk.device);
  params.append(disk.read_bytes_per_sec);
  params.append(disk.write_bytes_per_sec);

  db_.executeParams(
      "INSERT INTO metrics_disk "
      "  (time, agent_id, device, read_bytes_per_sec, write_bytes_per_sec) "
      "VALUES ($1, $2, $3, $4, $5)",
      params);
}

void MetricsRepository::saveNetMetric(const std::string& agent_id,
                                      const std::string& timestamp,
                                      const NetSample& interface) {
  pqxx::params params;
  params.append(timestamp);
  params.append(agent_id);
  params.append(interface.iface);
  params.append(interface.rx_bytes_per_sec);
  params.append(interface.tx_bytes_per_sec);

  db_.executeParams(
      "INSERT INTO metrics_net "
      "  (time, agent_id, iface, rx_bytes_per_sec, tx_bytes_per_sec) "
      "VALUES ($1, $2, $3, $4, $5)",
      params);
}

void MetricsRepository::saveNetMetrics(
    const std::string& agent_id, const std::string& timestamp,
    const std::vector<NetSample>& interfaces) {
  for (const auto& iface : interfaces) {
    saveNetMetric(agent_id, timestamp, iface);
  }
}

MetricsSample MetricsRepository::reconstructSample(
    const std::string& agent_id, const std::string& timestamp) {
  MetricsSample sample;
  sample.timestamp = timestamp;

  // Get CPU + Memory
  pqxx::params base_params;
  base_params.append(agent_id);
  base_params.append(timestamp);

  pqxx::result base = db_.executeParams(
      "SELECT cpu_total_percent, cpu_core_count, cpu_per_core, "
      "       mem_phys_total, mem_phys_used, mem_phys_available, "
      "       mem_swap_total, mem_swap_used "
      "FROM metrics WHERE agent_id = $1 AND time = $2",
      base_params);

  if (!base.empty()) {
    sample.cpu.total_percent = base[0]["cpu_total_percent"].as<float>();
    sample.cpu.per_core =
        deserializeCpuPerCore(base[0]["cpu_per_core"].as<std::string>());
    sample.mem.phys_total = base[0]["mem_phys_total"].as<std::uint64_t>();
    sample.mem.phys_used = base[0]["mem_phys_used"].as<std::uint64_t>();
    sample.mem.phys_available =
        base[0]["mem_phys_available"].as<std::uint64_t>();
    sample.mem.swap_total = base[0]["mem_swap_total"].as<std::uint64_t>();
    sample.mem.swap_used = base[0]["mem_swap_used"].as<std::uint64_t>();
  }

  getDiskSamples(sample, agent_id, timestamp);
  getNetSamples(sample, agent_id, timestamp);

  return sample;
}

void MetricsRepository::getDiskSamples(MetricsSample& sample,
                                       const std::string& agent_id,
                                       const std::string& timestamp) {
  // Get disks
  pqxx::params disk_params;
  disk_params.append(agent_id);
  disk_params.append(timestamp);

  pqxx::result disks = db_.executeParams(
      "SELECT device, read_bytes_per_sec, write_bytes_per_sec "
      "FROM metrics_disk WHERE agent_id = $1 AND time = $2",
      disk_params);

  for (const auto& row : disks) {
    DiskSample disk;
    disk.device = row["device"].as<std::string>();
    disk.read_bytes_per_sec = row["read_bytes_per_sec"].as<float>();
    disk.write_bytes_per_sec = row["write_bytes_per_sec"].as<float>();
    sample.disks.push_back(disk);
  }
}

void MetricsRepository::getNetSamples(MetricsSample& sample,
                                      const std::string& agent_id,
                                      const std::string& timestamp) {
  // Get interfaces
  pqxx::params net_params;
  net_params.append(agent_id);
  net_params.append(timestamp);

  pqxx::result nets = db_.executeParams(
      "SELECT iface, rx_bytes_per_sec, tx_bytes_per_sec "
      "FROM metrics_net WHERE agent_id = $1 AND time = $2",
      net_params);

  for (const auto& row : nets) {
    NetSample net;
    net.iface = row["iface"].as<std::string>();
    net.rx_bytes_per_sec = row["rx_bytes_per_sec"].as<float>();
    net.tx_bytes_per_sec = row["tx_bytes_per_sec"].as<float>();
    sample.interfaces.push_back(net);
  }
}

std::vector<MetricsSample> MetricsRepository::findByAgent(
    const std::string& agent_id, const std::string& since_iso,
    const std::string& until_iso) {
  pqxx::params params;
  params.append(agent_id);
  params.append(since_iso);
  params.append(until_iso);

  pqxx::result timestamps = db_.executeParams(
      "SELECT DISTINCT time FROM metrics "
      "WHERE agent_id = $1 AND time >= $2 AND time <= $3 "
      "ORDER BY time DESC",
      params);

  std::vector<MetricsSample> results;
  results.reserve(timestamps.size());

  for (const auto& row : timestamps) {
    std::string time = row["time"].as<std::string>();
    results.push_back(reconstructSample(agent_id, time));
  }

  return results;
}

std::optional<MetricsSample> MetricsRepository::getLatest(
    const std::string& agent_id) {
  pqxx::params params;
  params.append(agent_id);

  pqxx::result row = db_.executeParams(
      "SELECT time FROM metrics WHERE agent_id = $1 "
      "ORDER BY time DESC LIMIT 1",
      params);

  if (row.empty()) {
    return std::nullopt;
  }

  std::string timestamp = row[0]["time"].as<std::string>();
  return reconstructSample(agent_id, timestamp);
}

std::string MetricsRepository::serializeCpuPerCore(
    const std::vector<float>& per_core) {
  if (per_core.empty()) {
    return "{}";
  }

  std::ostringstream oss;
  oss << "{";
  for (std::size_t i = 0; i < per_core.size(); ++i) {
    if (i > 0) oss << ",";
    oss << per_core[i];
  }
  oss << "}";
  return oss.str();
}

std::vector<float> MetricsRepository::deserializeCpuPerCore(
    const std::string& arrayStr) {
  std::vector<float> result;

  // Remove outer braces: "{1.5,2.3,1.8}" → "1.5,2.3,1.8"
  if (arrayStr.size() < 2) return result;

  std::string inner = arrayStr.substr(1, arrayStr.size() - 2);
  if (inner.empty()) return result;

  std::istringstream iss(inner);
  std::string token;
  while (std::getline(iss, token, ',')) {
    try {
      result.push_back(std::stof(token));
    } catch (...) {
      // Skip invalid values
    }
  }

  return result;
}