#include "processtablewidget.h"
#include <QProgressBar>

#include <QVBoxLayout>
#include <QGridLayout>
#include "uiutils.h"

#include <QLabel>
#include <QLayoutItem>
#include <algorithm>


ProcessTableWidget::ProcessTableWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *outer = new QVBoxLayout(this);

    outer->addWidget(makeLabel("RUNNING PROCESSES (TOP 10 BY MEMORY)", "sectionTitle"));


    QWidget *grid = new QWidget;
    grid->setObjectName("processGrid");
    m_grid = new QGridLayout(grid);

    // Header row (row 0) — static column labels, keep as-is.
    const QStringList headers = { "PID", "Name", "CPU %", "CPU bar", "Mem %", "Mem bar", "Status" };
    for (int col = 0; col < headers.size(); ++col)
        m_grid->addWidget(makeLabel(headers[col], "processHeaderCell"), 0, col);

    // TODO: replace this hardcoded vector with the process list from the
    // server ProcessListPayload. Map each incoming entry to a ProcessInfo;
    // the fill loop below (labels + createBar) stays unchanged.
    const QVector<ProcessRow> processes = {
        { "1284", "nginx", "18%", "2.1%", "running", 18, 2 },
        { "3041", "postgres", "12%", "8.4%", "running", 12, 8 },
        { "887", "python3", "9%", "3.2%", "running", 70, 3 },
        { "512", "systemd", "0.4%", "0.8%", "running", 1, 1 }
    };


    // Fill loop: one grid row per process. Column order matches the header above.
    int row = 1;


    for (const ProcessRow &p : processes) {
        m_grid->addWidget(makeLabel(p.pid, "processPid"), row, 0);
        m_grid->addWidget(makeLabel(p.name, "processCell"), row, 1);
        m_grid->addWidget(makeLabel(p.cpuPercent, "processCell"), row, 2);
        m_grid->addWidget(createBar(p.cpuValue), row, 3);
        m_grid->addWidget(makeLabel(p.memPercent, "processCell"), row, 4);
        m_grid->addWidget(createBar(p.memValue), row, 5);
        m_grid->addWidget(makeLabel(p.status, "processStatus"), row, 6);
        ++row;

        // Separator line spanning all 7 columns
        m_grid->addWidget(createSeparator(), row, 0, 1, 7);
        ++row;
    }

    // Footer row: hint that more rows exist (static, matches the mockup).
    m_grid->addWidget(makeLabel("...  more rows", "processFooter"), row, 0, 1, 3);
    m_grid->addWidget(makeLabel("scroll to load more", "processFooter"), row, 4, 1, 3);

    outer->addWidget(grid);


    setProcesses({});


}



QWidget *ProcessTableWidget::createSeparator()
{
    QWidget *line = new QWidget;
    line->setObjectName("processSeparator");
    line->setFixedHeight(1);
    return line;
}



QWidget *ProcessTableWidget::createBar(int value)
{
    QProgressBar *bar = new QProgressBar;
    bar->setObjectName("processBar");
    bar->setRange(0, 100);
    bar->setValue(value);
    bar->setTextVisible(false);
    bar->setFixedHeight(6);
    bar->setMaximumWidth(140);
    bar->setProperty("high", value >= 50);   // ← more when >=50 the color change
    return bar;
}

void ProcessTableWidget::setPhysTotal(std::uint64_t physTotal)
{
    m_physTotal = physTotal;
}

void ProcessTableWidget::setProcesses(const std::vector<ProcessInfo> &processes)
{
    // Remove every widget currently in the grid.a
    while (QLayoutItem *item = m_grid->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const QStringList headers = { "PID", "Name", "CPU %", "CPU bar", "Mem", "Mem bar", "Status" };
    for (int col = 0; col < headers.size(); ++col)
        m_grid->addWidget(makeLabel(headers[col], "processHeaderCell"), 0, col);


    std::vector<ProcessInfo> sorted = processes;
    std::sort(sorted.begin(), sorted.end(),
              [](const ProcessInfo &a, const ProcessInfo &b) {
                  return a.mem_bytes > b.mem_bytes;
              });

    int row = 1;
    const int count = qMin(10, static_cast<int>(sorted.size()));
    for (int i = 0; i < count; ++i) {
        const ProcessInfo &p = sorted[i];
        ProcessRow r;
        r.pid = QString::number(p.pid);
        r.name = QString::fromStdString(p.name);
        r.cpuPercent = QString::number(p.cpu_percent, 'f', 1) + "%";
        r.cpuValue = static_cast<int>(p.cpu_percent);
        r.memPercent = QString::number(p.mem_bytes / 1024.0, 'f', 1) + " MB";
        r.memValue = m_physTotal == 0
                         ? 0
                         : static_cast<int>(p.mem_bytes * 1024.0 * 100.0 / m_physTotal);
        r.status = "running";        // TODO: not provided by the protocol

        m_grid->addWidget(makeLabel(r.pid, "processPid"), row, 0);
        m_grid->addWidget(makeLabel(r.name, "processCell"), row, 1);
        m_grid->addWidget(makeLabel(r.cpuPercent, "processCell"), row, 2);
        m_grid->addWidget(createBar(r.cpuValue), row, 3);
        m_grid->addWidget(makeLabel(r.memPercent, "processCell"), row, 4);
        m_grid->addWidget(createBar(r.memValue), row, 5);
        m_grid->addWidget(makeLabel(r.status, "processStatus"), row, 6);
        ++row;

        m_grid->addWidget(createSeparator(), row, 0, 1, 7);
        ++row;

    }
}
