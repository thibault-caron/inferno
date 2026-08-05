#include "serverclient.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QSocketNotifier>
#include <QFileInfo>

#include "codec/metrics_parser.hpp"
#include "codec/protocol_helper.hpp"
#include "codec/protocol_parser.hpp"
#include "codec/protocol_serializer.hpp"
#include "dashboardsession.h"
#include "env_helper.hpp"
#include "socket/socket_factory.hpp"
#include "socket/tls_socket_factory.hpp"

/// Returns the first existing path to the CA certificate, or an empty string.
static QString resolveCertPath() {

    const QString base = QCoreApplication::applicationDirPath();

    const QStringList candidates = {
        base + "/../../certs/ca.crt",
        base + "/../../../certs/ca.crt",
        base + "/certs/ca.crt"
    };

    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            qDebug() << "using certificate:" << path;
            return path;
        }
    }

    qDebug() << "no certificate found";


    return QString();
}

static QString osTypeToString(OSType type) {
    switch (type) {
    case OSType::WINDOWS:
        return "Windows";
    case OSType::LINUX:
        return "Linux";
    case OSType::MAC:
        return "macOS";
    default:
        return "Unknown";
    }
}

static QString archToString(ArchType arch) {
    switch (arch) {
    case ArchType::X86:
        return "x86";
    case ArchType::X64:
        return "x64";
    case ArchType::ARM:
        return "ARM";
    default:
        return "Unknown";
    }
}

ServerClient::ServerClient(QObject* parent) : QObject{parent} {}

ServerClient::~ServerClient() = default;

bool ServerClient::connectToServer(const QString& host, quint16 port) {
    qDebug() << "before creating tls socket";
    bool tlsEnabled = EnvHelper::resolveTlsEnabled();
    std::unique_ptr<ISocket> socket;
    if (tlsEnabled) {
        qDebug() << "tls enabled";

        /*QString certPath =

        QCoreApplication::applicationDirPath() + "/../../certs/ca.crt";
    // certPath = QFileInfo(certPath).absoluteFilePath();
    socket = TLSSocketFactory::createClient(certPath.toStdString());*/

        const QString certPath = resolveCertPath();
        if (certPath.isEmpty()) {
            qDebug() << "cannot connect with TLS without a certificate";
            return false;
        }

        socket = TLSSocketFactory::createClient(certPath.toStdString());

    } else {
        qDebug() << "tls disabled";
        socket = SocketFactory::createTCP();
    }

    if (!socket) {
        qDebug() << "socket creation failed";
        return false;
    }

    if (!socket->connect(host.toStdString(), port)) {
        qDebug() << "connection failed";
        return false;
    }

    socket->setNonBlocking(true);
    m_session = std::make_unique<DashboardSession>(std::move(socket));

    qDebug() << "connected to server";
    m_notifier =
        new QSocketNotifier(m_session->getFd(), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this,
            &ServerClient::onReadyRead);

    sendRegister();
    return true;
}

void ServerClient::onReadyRead() {
    // qDebug() << "data available";
    const SocketResult result = m_session->receiveIntoBuffer();

    // nothing to read for this moment
    if (!result.ok() && !result.wouldBlock()) {
        // qDebug() << "receive error";
        return;
    }

    // TODO: result.ok() with 0 bytes means the server closed the connection.

    while (std::optional<Frame> frame = m_session->tryExtractFrame()) {
        handleFrame(*frame);
    }
}

void ServerClient::sendRegister() {
    OsInfoPayload info;

#ifdef _WIN32
    info.os_type = OSType::WINDOWS;
    info.os_version = "Windows";
    QString user = qEnvironmentVariable("USERNAME");
#else
    info.os_type = OSType::LINUX;
    info.os_version = "Linux";
    QString user = qEnvironmentVariable("USER");
#endif

    if (user.isEmpty()) user = "dashboard";

    info.arch = ArchType::X64;
    info.hostname = QHostInfo::localHostName().toStdString();
    info.current_user = user.toStdString();
    // info.os_version = "Windows";
    //  TODO: use the real local IP (QNetworkInterface).
    info.ip = "127.0.0.1";
    // The server uses this field as our id (setId(mac)).
    // TODO: replace with the real MAC address.
    info.mac = "dashboard";

    try {
        const std::vector<std::uint8_t> payload =
            ProtocolSerializer::serializeOsInfoPayload(info);

        const Frame frame{
                          ProtocolHelper::createHeader(MessageType::DASHBOARD_REGISTER, payload),
                          payload};

        m_session->sendFrame(frame);
    } catch (const std::exception& e) {
        // std::cerr << e.what() << '\n';
        qDebug() << "register failed:" << e.what();
    }
}

void ServerClient::handleFrame(const Frame& frame) {
    switch (frame.header.type) {
    case MessageType::DATA:
        qDebug() << "DATA received," << frame.payload.size() << "bytes";
        handleData(frame.payload);
        break;
    case MessageType::RESPONSE: {
        const DashboardResponse response =
            ProtocolParser::parseDashboardResponse(frame.payload);

        const QString target = QString::fromStdString(response.target);
        const CommandType type = response.response.type;
        qDebug() << "RESPONSE target:" << target
                 << "| id:" << response.response.id
                 << "| type:" << static_cast<int>(type);
                 // << "| cles:" << m_lastCommandByTarget.keys();


        bool parsed = false;
        if (type == CommandType::RUNNING_PROCESSES) {
            try {
                const std::vector<ProcessInfo> processes =
                    ProtocolParser::parseProcessInfoList(response.response.data);
                qDebug() << "process list:" << processes.size() << "entries";
                emit processListReceived(target, processes);
                parsed = true;
            } catch (const std::exception& e) {
                qDebug() << "not a process list:" << e.what();
            }
        }

        if (type == CommandType::OS_INFO) {
            try {
                const OsInfoPayload info =
                    ProtocolParser::parseOsInfoPayload(response.response.data);
                emit osInfoReceived(target, info);
                parsed = true;
            } catch (const std::exception& e) {
                qDebug() << "not an os info payload:" << e.what();
            }
        }

        if (!parsed) {
            emit responseReceived(
                target, QString::fromStdString(
                    ProtocolParser::toString(response.response.data)));
        }
        break;
    }
    case MessageType::INFERNO_ERROR: {
        const ErrorPayload error =
            ProtocolParser::parseErrorPayload(frame.payload);
        qDebug() << "ERROR" << static_cast<int>(error.code)
                 << QString::fromStdString(error.message);
        break;
    }
    case MessageType::DISCONNECT: {
        try {
            const DashboardDisconnect payload =
                ProtocolParser::parseDashboardDisconnect(frame.payload);

            emit agentDisconnected(QString::fromStdString(payload.target));
        } catch (const std::exception& e) {
            qDebug() << "bad disconnect payload:" << e.what();
        }
        break;
    }

    default:
        qDebug() << "unhandled type" << static_cast<int>(frame.header.type);
    }
}

void ServerClient::handleData(const std::vector<std::uint8_t>& payload) {
    // const DataPayload data = ProtocolParser::parseDataPayload(payload);
    // const DashboardData dataDashboard =
    //     ProtocolParser::parseDashboardData(payload);
    DashboardData dataDashboard;
    try {
        dataDashboard = ProtocolParser::parseDashboardData(payload);
    } catch (const std::exception& e) {
        qDebug() << "parseDashboardData failed:" << e.what();
        return;
    }

    const std::string agentId = dataDashboard.target;
    const DataPayload data = dataDashboard.data;

    qDebug() << "DATA subtype" << static_cast<int>(data.subtype)
             << "size" << data.data.size();

    switch (data.subtype) {
    case DataType::AGENTS:
    case DataType::REGISTRATION: {
        if (data.data.empty()) break;  // no agent connected yet

        std::vector<RegisterPayload> agents;
        if (data.subtype == DataType::AGENTS) {
            agents = ProtocolParser::parseRegisterPayloadList(data.data);
        } else {
            agents.push_back(ProtocolParser::parseRegisterPayload(data.data));
        }

        qDebug() << "parsed agents:" << agents.size();
        for (const RegisterPayload& agent : agents) {
            qDebug() << "agent:" << QString::fromStdString(agent.system.hostname)
            << QString::fromStdString(agent.system.ip)
            << "id:" << QString::fromStdString(agent.id)
            << "online:" << agent.online;

            // ... emit signals for each agent
            const std::string id = agent.id.empty() ? agent.system.mac : agent.id;

            const QString os = QString("%1 · %2")
                                   .arg(osTypeToString(agent.system.os_type),
                                        archToString(agent.system.arch));

            emit agentReceived(QString::fromStdString(id),
                               QString::fromStdString(agent.system.hostname), os,
                               QString::fromStdString(agent.system.ip),
                               agent.online);


        }

        break;
    }
    case DataType::METRICS_SAMPLE: {
        const MetricsSample sample = MetricsParser::parseMetricsSample(data.data);


        emit metricsReceived(QString::fromStdString(agentId), sample);
        break;
    }
    default:
        qDebug() << "unhandled subtype" << static_cast<int>(data.subtype);
    }
}

void ServerClient::sendCommand(const QString& target, CommandType type,
                               const QString& data) {
    if (!m_session || target.isEmpty()) {
        qDebug() << "no agent selected";
        return;
    }

    if (type == CommandType::SHELL && data.isEmpty()) {
        qDebug() << "empty shell command";
        return;
    }

    CommandPayload command;
    command.id = ++m_nextCommandId;
    command.type = type;
    command.data = data.toStdString();

    DashboardCommand dashCommand;
    dashCommand.target = target.toStdString();
    dashCommand.command = command;

    try {
        const std::vector<std::uint8_t> payload =
            ProtocolSerializer::serializeDashboardCommand(dashCommand);

        const Frame frame{
                          ProtocolHelper::createHeader(MessageType::COMMAND, payload), payload};

        m_session->sendFrame(frame);
        // m_lastCommandByTarget.insert(target, type); // suppr this
        qDebug() << "sent command id" << command.id << "to" << target;
    } catch (const std::exception& e) {
        qDebug() << "send command failed:" << e.what();
    }
}

void ServerClient::sendDisconnect(const QString& target) {
    if (!m_session || target.isEmpty()) {
        qDebug() << "no agent selected";
        return;
    }

    DashboardDisconnect payload;
    payload.target = target.toStdString();

    try {
        const std::vector<std::uint8_t> data =
            ProtocolSerializer::serializeDashboardDisconnect(payload);

        const Frame frame{
                          ProtocolHelper::createHeader(MessageType::DISCONNECT, data), data};

        m_session->sendFrame(frame);
        qDebug() << "sent disconnect to" << target;
    } catch (const std::exception& e) {
        qDebug() << "send disconnect failed:" << e.what();
    }
}
