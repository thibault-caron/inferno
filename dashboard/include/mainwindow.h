#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVector>

#include <QDateTime>
#include <QTimer>

#include "protocol/lptf_protocol.hpp"
#include "serieshistory.h"

class QLabel;
class LineChartWidget;
class ServerClient;
class ProcessTableWidget;
class MetricCardsWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    /// Fills the metric cards with the latest sample.
    void onMetricsReceived(const QString &target, const MetricsSample &sample);

    /// Fills the OS badge and the console from an OS_INFO answer.
    void onOsInfoReceived(const QString &target, const OsInfoPayload &info);

    /// Marks an agent as offline when the server reports its disconnection.
    void onAgentDisconnected(const QString &target);

    /// Rewrites the status bar label with the age of the last sample.
    void updateLastSampleLabel();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    ServerClient* m_client = nullptr;

    Ui::MainWindow* ui;
    /// Fills the agent list. TODO: replace hardcoded data with server AGENTS
    /// payload.
    void populateAgents();

    /// Adds one rich agent row to the list.
    void addAgentItem(const QString& id, const QString& name, const QString& os,
                      const QString& ip, bool online);

    /// Displays command output. TODO: feed with DashboardResponse data.
    void showOutput(const QString& text);

    /// Empties every widget that shows agent data.
    void clearAgentView();

    /// Recomputes the agent counters shown in the sidebar.
    void updateAgentCounters();

    /// Refreshes the header status badge from the current streaming state.
    void updateStatusBadge();

    /// Builds the metric cards, table and console inside contentArea
    void buildContentArea();

    /// Creates one metric card (title + big value + subtitle).
    QWidget* createMetricCard(const QString& key, const QString& title,
                              const QString& value, const QString& subtitle);

    LineChartWidget* createChart(const QString& title,
                                 const QVector<QVector<double>>& series,
                                 const QStringList& labels,
                                 const QVector<int>& dashed = {},
                                 const QVector<int>& filled = {},
                                 const QString& topRight = {});

    QString m_target;

    QString m_streamingTarget;

    bool m_osBadgeDetailed = false;

    QHash<QString, QLabel*> m_metricValues;

    /// Updates a metric card value by key. TODO: call from server DataPayload.
    void updateMetric(const QString& key, const QString& value);

    /// Builds one process table row from a ProcessInfo.
    // QWidget *createProcessRow(const ProcessInfo &process, bool isHeader =
    // false);

    /// Builds the RUNNING PROCESSES section (title + header + rows).
    QWidget* createProcessTable();

    /// Creates a mini progress bar (0-100) for the process table.
    QWidget* createBar(int value);

    /// Creates a thin horizontal separator line.
    QWidget* createSeparator();

    /// Fills the bottom status bar. TODO: feed with live server status.
    void buildStatusBar();

    ProcessTableWidget* m_processTable = nullptr;

    MetricCardsWidget* m_metricCards = nullptr;

    LineChartWidget* m_memoryChart = nullptr;

    LineChartWidget* m_cpuChart = nullptr;

    LineChartWidget* m_diskChart = nullptr;

    LineChartWidget* m_networkChart = nullptr;

    QLabel* m_onlineLabel = nullptr;

    QLabel* m_lastSampleLabel = nullptr;
    QDateTime m_lastSampleTime;

    QTimer* m_sampleAgeTimer = nullptr;

    SeriesHistory m_memoryHistory{1, 20}; //20 historics points

    SeriesHistory m_diskHistory{2, 20};


    SeriesHistory m_networkHistory{2, 20};

    SeriesHistory m_cpuHistory{4, 20};



};

#endif  // MAINWINDOW_H
