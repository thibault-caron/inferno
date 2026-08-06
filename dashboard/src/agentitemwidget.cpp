#include "agentitemwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QStyle>

AgentItemWidget::AgentItemWidget(QWidget *parent)
    : QWidget(parent)
    , m_nameLabel(new QLabel(this))
    , m_detailsLabel(new QLabel(this))
    , m_statusDot(new QLabel(this))
{
    m_statusDot->setObjectName("agentStatusDot");
    m_statusDot->setText("●");
    m_nameLabel->setObjectName("agentItemName");
    m_detailsLabel->setObjectName("agentItemDetails");

    QVBoxLayout *textColumn = new QVBoxLayout;
    textColumn->addWidget(m_nameLabel);
    textColumn->addWidget(m_detailsLabel);

    QHBoxLayout *row = new QHBoxLayout(this);
    row->addWidget(m_statusDot);
    row->addLayout(textColumn);
    row->addStretch();
}

void AgentItemWidget::setAgent(const QString &name, const QString &details, bool online)
{
    m_nameLabel->setText(name);
    m_detailsLabel->setText(details);
    m_statusDot->setProperty("online", online);
    // m_statusDot->style()->unpolish(m_statusDot);
    // m_statusDot->style()->polish(m_statusDot);
}

void AgentItemWidget::setOnline(bool online)
{
    m_statusDot->setProperty("online", online);
    m_statusDot->style()->unpolish(m_statusDot);
    m_statusDot->style()->polish(m_statusDot);
}
