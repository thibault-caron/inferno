#ifndef METRICCARDSWIDGET_H
#define METRICCARDSWIDGET_H

#include <QHash>
#include <QString>
#include <QWidget>

#include "protocol/lptf_protocol.hpp"

class QLabel;

class MetricCardsWidget : public QWidget {
  Q_OBJECT
 public:
  explicit MetricCardsWidget(QWidget* parent = nullptr);

  /// Updates a metric card value by key. TODO: call from server DataPayload.
  void updateMetric(const QString& key, const QString& value);

  /// Fills every card from one metrics sample.
  void updateFromSample(const MetricsSample& sample);

  /// Updates a metric card subtitle by key.
  void updateSubtitle(const QString &key, const QString &value);

  /// Resets every card to a placeholder value.
  void clear();

 private:
  /// Creates one metric card (title + big value + subtitle).
  QWidget* createMetricCard(const QString& key, const QString& title,
                            const QString& value, const QString& subtitle);

  QHash<QString, QLabel*> m_metricValues;

  QHash<QString, QLabel *> m_metricSubtitles;


};

#endif  // METRICCARDSWIDGET_H
