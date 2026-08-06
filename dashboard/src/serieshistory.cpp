#include "serieshistory.h"


SeriesHistory::SeriesHistory(int seriesCount, int maxPoints)
    : m_maxPoints(maxPoints) {
    m_series.resize(seriesCount);
}


void SeriesHistory::append(const QVector<double>& values) {
    for (int i = 0; i < m_series.size(); ++i) {
        m_series[i].append(values[i]);

        if (m_series[i].size() > m_maxPoints) m_series[i].removeFirst();
    }
}

QVector<QVector<double>> SeriesHistory::series() const { return m_series; }

void SeriesHistory::clear() {
    for (int i = 0; i < m_series.size(); ++i) m_series[i].clear();
}




