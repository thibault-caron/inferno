#ifndef AGENTITEMWIDGET_H
#define AGENTITEMWIDGET_H

#include <QWidget>

class QLabel;

class AgentItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AgentItemWidget(QWidget *parent = nullptr);

    void setAgent(const QString &name, const QString &details, bool online);
    /// Updates only the connection state dot.
    void setOnline(bool online);

private:
    QLabel *m_nameLabel;
    QLabel *m_detailsLabel;
    QLabel *m_statusDot;
};

#endif // AGENTITEMWIDGET_H
