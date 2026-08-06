#include "metriccardswidget.h"
#include "uiutils.h"

#include <QLabel>
#include <QString>
#include <QStringList>
#include <QFrame>
#include <QDebug>
#include <QVBoxLayout>
#

MetricCardsWidget::MetricCardsWidget(QWidget *parent)
    : QWidget{parent}
{
    QHBoxLayout *row = new QHBoxLayout(this);

    // TODO: replace hardcoded values with live server DataPayload.
    row->addWidget(createMetricCard("cpu", "CPU overall", "34%", "4 cores"));
    row->addWidget(createMetricCard("memory", "Memory used", "5.2 GB", "of 16 GB · swap 0.1"));
    row->addWidget(createMetricCard("disk", "Disk read", "12 MB/s", "write 4 MB/s"));
    row->addWidget(createMetricCard("network", "Network rx", "820 KB/s", "tx 210 KB/s"));

    clear();
}


void MetricCardsWidget::updateMetric(const QString &key, const QString &value)
{
    if (m_metricValues.contains(key))
        m_metricValues[key]->setText(value);
}


QWidget *MetricCardsWidget::createMetricCard(const QString &key, const QString &title, const QString &value, const QString &subtitle)
{
    QFrame *card = new QFrame;
    card->setObjectName("metricCard");

    QLabel *valueLabel = makeLabel(value, "metricValue");
    m_metricValues.insert(key, valueLabel);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->addWidget(makeLabel(title, "metricTitle"));
    layout->addWidget(valueLabel);

    QLabel *subtitleLabel = makeLabel(subtitle, "metricSubtitle");
    m_metricSubtitles.insert(key, subtitleLabel);
    layout->addWidget(subtitleLabel);

    return card;
}

void MetricCardsWidget::updateFromSample(const MetricsSample &sample)
{
    updateMetric("cpu", QString::number(sample.cpu.total_percent, 'f', 1) + "%");

    updateSubtitle("cpu",
                   QString::number(static_cast<int>(sample.cpu.per_core.size()))
                       + " cores");

    const double gb = sample.mem.phys_used / 1024.0 / 1024.0 / 1024.0;
    updateMetric("memory", QString::number(gb, 'f', 1) + " GB");

    const double totalGb = sample.mem.phys_total / 1024.0 / 1024.0 / 1024.0;
    const double swapGb = sample.mem.swap_used / 1024.0 / 1024.0 / 1024.0;
    updateSubtitle("memory", "of " + QString::number(totalGb, 'f', 1)
                                 + " GB · swap "
                                 + QString::number(swapGb, 'f', 1));

    double diskRead = 0.0;
    for (const DiskSample &disk : sample.disks) diskRead += disk.read_bytes_per_sec;

    diskRead = diskRead / 1024.0 / 1024.0;
    updateMetric("disk", QString::number(diskRead, 'f', 1) + " MB/s");

    double diskWrite = 0.0;
    for (const DiskSample &disk : sample.disks) diskWrite += disk.write_bytes_per_sec;

    diskWrite = diskWrite / 1024.0 / 1024.0;
    updateSubtitle("disk", "write " + QString::number(diskWrite, 'f', 1) + " MB/s");


    double netRx = 0.0;
    double netTx = 0.0;
    for (const NetSample &iface : sample.interfaces) {
        netRx += iface.rx_bytes_per_sec;
        netTx += iface.tx_bytes_per_sec;
    }

    const double maxRate = 1024.0 * 1024.0 * 1024.0;
    if (netRx > maxRate || netTx > maxRate) {
        qDebug() << "network rate out of range, sample skipped:" << netRx << netTx;
    } else {
        updateMetric("network", QString::number(netRx / 1024.0, 'f', 1) + " KB/s");
        updateSubtitle("network", "tx " + QString::number(netTx / 1024.0, 'f', 1) + " KB/s");
    }
}

void MetricCardsWidget::updateSubtitle(const QString &key, const QString &value)
{
    if (m_metricSubtitles.contains(key))
        m_metricSubtitles[key]->setText(value);


}


void MetricCardsWidget::clear()
{
    const QStringList keys = {"cpu", "memory", "disk", "network"};

    for (const QString &key : keys) {
        updateMetric(key, "—");
        updateSubtitle(key, "—");
    }
}
