#ifndef LINECHARTWIDGET_H
#define LINECHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>

class LineChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineChartWidget(QWidget *parent = nullptr);
    void setData(const QVector<double> &data);
    void setTitle(const QString &title);
    void setSeries(const QVector<QVector<double>> &series);
    void setLabels(const QStringList &labels);
    void setDashed(const QVector<int> &indices);
    void setFilled(const QVector<int> &indices);
    void setTopRight(const QString &text);

    /// Sets the top of the Y axis. Default is 100.
    void setYMax(double max);

    /// Sets the suffix shown on Y labels. Default is "%".
    void setYUnit(const QString &unit);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_data; // a list of number -> one one ligne
    QVector<QVector<double>> m_series;   // one inner vector per curve
    QString m_title;
    QStringList m_labels;   // one name per series, for the legend
    QVector<int> m_dashed;   // indices of series drawn with a dashed pen
    QVector<int> m_filled;   // indices of series drawn with a filled area
    QString m_topRight;   // small text shown top-right (e.g. "16 GB total")

    double m_yMax = 100.0;
    QString m_yUnit = "%";
};

#endif // LINECHARTWIDGET_H
