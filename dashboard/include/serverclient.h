#ifndef SERVERCLIENT_H
#define SERVERCLIENT_H
#include <QHash>
#include <QObject>
#include <memory>

#include "protocol/lptf_protocol.hpp"

class DashboardSession;
class QSocketNotifier;

/// Owns the TCP connection to the Inferno server.
class ServerClient : public QObject {
  Q_OBJECT

 public:
  explicit ServerClient(QObject* parent = nullptr);
  ~ServerClient();

  /// Connects to the server and starts listening for frames.
  bool connectToServer(const QString& host, quint16 port);
  void sendCommand(const QString& target, CommandType type,
                   const QString& data);

  /// Asks the server to disconnect an agent.
  void sendDisconnect(const QString& target);

 signals:
  void agentReceived(const QString& id, const QString& name,const QString& os,
                     const QString& ip, bool online);
  void responseReceived(const QString& target, const QString& text);
  void processListReceived(const QString& target,
                           const std::vector<ProcessInfo>& processes);
  /// Emitted once per metrics sample received from an agent.
  void metricsReceived(const QString& target, const MetricsSample& sample);

  /// Emitted when an agent answers an OS_INFO command.
  void osInfoReceived(const QString& target, const OsInfoPayload& info);

  /// Emitted when the server reports that an agent went offline.
  void agentDisconnected(const QString& target);

 private:
  std::unique_ptr<DashboardSession> m_session;
  QSocketNotifier* m_notifier = nullptr;
  std::uint32_t m_nextCommandId = 0;
  // QHash<QString, CommandType> m_lastCommandByTarget;
  void onReadyRead();
  void sendRegister();
  void handleFrame(const Frame& frame);
  void handleData(const std::vector<std::uint8_t>& payload);
};

#endif  // SERVERCLIENT_H
