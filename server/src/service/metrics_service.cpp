#include "service/metrics_service.hpp"

#include "codec/metrics_serializer.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_serializer.hpp"

Frame MetricsService::save(const std::string& agent_id,
                           const MetricsSample& sample) {
  DashboardData data;
  data.target = agent_id;
  repository_.save(agent_id, sample);
  // what << "[DATA] METRICS_SAMPLE\n";

  // what << "CPU: " << sample.cpu.total_percent << "%\n";
  // what << "CPU cores: ";
  // for (float core : sample.cpu.per_core) what << core << "% ";
  // what << '\n';

  // what << "Memory: " << sample.mem.phys_used << "/" << sample.mem.phys_total
  //      << " bytes used\n";

  // for (const auto& disk : sample.disks) {
  //   what << "Disk " << disk.device << " R=" << disk.read_bytes_per_sec
  //        << " W=" << disk.write_bytes_per_sec << '\n';
  // }

  // for (const auto& iface : sample.interfaces) {
  //   what << "Net " << iface.iface << " RX=" << iface.rx_bytes_per_sec
  //        << " TX=" << iface.tx_bytes_per_sec << '\n';
  // }

  data.data.subtype = DataType::METRICS_SAMPLE;
  data.data.data = MetricsSerializer::serializeMetricsSample(sample);
  Frame frame;
  frame.payload = ProtocolSerializer::serializeDashboardData(data);
  frame.header = ProtocolHelper::createHeader(MessageType::DATA, frame.payload);
  return frame;
}