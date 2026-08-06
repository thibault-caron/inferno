#ifndef PROCESSTABLEWIDGET_H
#define PROCESSTABLEWIDGET_H

#include <QWidget>
#include <QString>
#include <vector>
#include <cstdint>

#include "protocol/lptf_protocol.hpp"


class QGridLayout;

class ProcessTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProcessTableWidget(QWidget *parent = nullptr);

    /// Replaces the table content with the given processes.
    void setProcesses(const std::vector<ProcessInfo> &processes);
    void setPhysTotal(std::uint64_t physTotal);

private:
    struct ProcessRow {
        QString pid;
        QString name;
        QString cpuPercent;  // display text, e.g. "18%"
        QString memPercent;  // display text, e.g. "2.1%"
        QString status;
        int cpuValue = 0;    // 0-100, drives the CPU bar
        int memValue = 0;    // 0-100, drives the Mem bar
    };

    /// Creates a thin horizontal separator line.
    QWidget *createSeparator();

    /// Creates a mini progress bar (0-100) for the process table.
    QWidget *createBar(int value);

    QGridLayout *m_grid = nullptr;
    std::uint64_t m_physTotal = 0;
};

#endif // PROCESSTABLEWIDGET_H
