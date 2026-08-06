#include "uiutils.h"

#include <QLabel>

QLabel *makeLabel(const QString &text, const QString &objectName)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(objectName);
    return label;
}
