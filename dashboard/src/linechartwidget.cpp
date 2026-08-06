#include "linechartwidget.h"
#include "theme.h"

#include <QPainter>

LineChartWidget::LineChartWidget(QWidget *parent)
    : QWidget(parent)
{
}

void LineChartWidget::setData(const QVector<double> &data)
{
    m_series = { data };
    update();
}

void LineChartWidget::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void LineChartWidget::setSeries(const QVector<QVector<double>> &series)
{
    m_series = series;
    update();
}

void LineChartWidget::setLabels(const QStringList &labels)
{
    m_labels = labels;
    update();
}
void LineChartWidget::setDashed(const QVector<int> &indices)
{
    m_dashed = indices;
    update();
}
void LineChartWidget::setFilled(const QVector<int> &indices)
{
    m_filled = indices;
    update();
}
void LineChartWidget::setTopRight(const QString &text)
{
    m_topRight = text;
    update();
}

void LineChartWidget::setYMax(double max)
{
    m_yMax = max;
    update();
}

void LineChartWidget::setYUnit(const QString &unit)
{
    m_yUnit = unit;
    update();
}

void LineChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    QRectF card = rect().adjusted(0, 0, -1, -1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::CardBackground);
    painter.drawRoundedRect(card, 12, 12);


    double w = width();
    double h = height();

    const double pad = 30.0;
    const double leftPad = 44.0;
    double plotLeft = leftPad;
    double plotTop = pad + 42;
    double plotWidth = w - leftPad - pad;
    double plotHeight = h - plotTop - pad;

    if (!m_title.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 1);
        painter.setFont(titleFont);
        painter.setPen(Theme::TitleText);
        painter.drawText(QRectF(plotLeft, 16, plotWidth, 18),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         m_title);
        painter.setFont(QFont());
    }

    if (!m_topRight.isEmpty()) {
        painter.setPen(Theme::MutedText);
        painter.drawText(QRectF(plotLeft, 16, plotWidth, 18),
                         Qt::AlignRight | Qt::AlignVCenter,
                         m_topRight);
    }

    // if no data no graph
    if (m_series.isEmpty())
        return;

    const QVector<QColor> palette = {
        Theme::Series0,
        Theme::Series1,
        Theme::Series2,
        Theme::Series3
    };

    double legendX = plotLeft;
    const double legendY = 48;
    for (int i = 0; i < m_labels.size(); ++i) {
        painter.setPen(QPen(palette[i % palette.size()], 2));
        painter.drawLine(QPointF(legendX, legendY), QPointF(legendX + 14, legendY));
        legendX += 20;

        painter.setPen(Theme::MutedText);
        QRectF textRect(legendX, legendY - 8, 60, 16);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_labels[i]);
        legendX += painter.fontMetrics().horizontalAdvance(m_labels[i]) + 16;
    }

    // but GB for memory, MB/s for I/O). Hardcoded to percentages for now.
    // double minVal = 0.0;
    // double maxVal = 100.0;
    const double minVal = 0.0;
    const double maxVal = m_yMax;

    //hache et graduation

    for (int i = 0; i <= 2; ++i) {
        double y = plotTop + (plotHeight / 2) * i;

        painter.setPen(QPen(Theme::GridLine, 1));
        painter.drawLine(QPointF(plotLeft, y), QPointF(plotLeft + plotWidth, y));

        // int percent = 100 - i * 50;
        // painter.setPen(Theme::MutedText);
        // painter.drawText(QRectF(0, y - 8, plotLeft - 4, 16),
        //                  Qt::AlignRight | Qt::AlignVCenter,
        //                  QString::number(percent) + "%");
        const double value = m_yMax - (m_yMax / 2) * i;
        painter.setPen(Theme::MutedText);
        painter.drawText(QRectF(0, y - 8, plotLeft - 4, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value, 'g', 3) + m_yUnit);
    }

    const QStringList xLabels = { "-20s", "-10s", "now" };
    painter.setPen(Theme::MutedText);
    for (int i = 0; i < xLabels.size(); ++i) {
        double x = plotLeft + (plotWidth / 2) * i;
        painter.drawText(QRectF(x - 30, plotTop + plotHeight + 4, 60, 16),
                         Qt::AlignCenter,
                         xLabels[i]);
    }


    painter.setBrush(Qt::NoBrush); // got out the for

    //
    for (int s = 0; s < m_series.size(); ++s) {
        const QVector<double> &serie = m_series[s];
        if (serie.size() < 2)
            continue;

        QPolygonF points;
        for (int i = 0; i < serie.size(); ++i) {
            double x = plotLeft + i * (plotWidth / (serie.size() - 1));
            const double value = qBound(minVal, serie[i], maxVal);
            double y = plotTop + plotHeight - (value - minVal) / (maxVal - minVal) * plotHeight;
            points << QPointF(x, y);
        }

        QPen pen(palette[s % palette.size()], 2);
        if (m_dashed.contains(s))
            pen.setStyle(Qt::DashLine);
        painter.setPen(pen);

        painter.drawPolyline(points);

        if (m_filled.contains(s)) {
            QPolygonF area = points;
            double bottom = plotTop + plotHeight;
            area << QPointF(plotLeft + plotWidth, bottom);
            area << QPointF(plotLeft, bottom);

            QColor fillColor = palette[s % palette.size()];
            fillColor.setAlpha(40);
            painter.setPen(Qt::NoPen);
            painter.setBrush(fillColor);
            painter.drawPolygon(area);
        }
    }


}
