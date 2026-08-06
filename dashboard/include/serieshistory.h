#ifndef SERIESHISTORY_H
#define SERIESHISTORY_H

#include <QVector>

class SeriesHistory {
public:
    SeriesHistory(int seriesCount, int maxPoints);

    /// Adds one point per curve, dropping the oldest when the limit is reached.
    void append(const QVector<double>& values);

    /// Empties every curve, keeping the curve count unchanged.
    void clear();


    /// Returns the stored points, ready for LineChartWidget::setSeries.
    QVector<QVector<double>> series() const;

private:
    /// One inner vector per curve, each holding that curve's latest points.
    QVector<QVector<double>> m_series;

     /// How many points each curve keeps before the oldest one is dropped
    int m_maxPoints = 20;
};

#endif  // SERIESHISTORY_H
