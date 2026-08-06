#ifndef UIUTILS_H
#define UIUTILS_H

#include <QString>

class QLabel;

/// Creates a QLabel with the given text and object name.
QLabel *makeLabel(const QString &text, const QString &objectName);


#endif // UIUTILS_H
