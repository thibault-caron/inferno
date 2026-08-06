#ifndef THEME_H
#define THEME_H

#include <QColor>

/// Centralized color palette for the dashboard.
namespace Theme {

// Chart series colors, in drawing order.
inline const QColor Series0 = QColor("#89B6A5");
inline const QColor Series1 = QColor("#48679c");
inline const QColor Series2 = QColor("#768eb6");
inline const QColor Series3 = QColor("#617ba8");

// Neutrals.
inline const QColor CardBackground = QColor(255, 255, 255);
inline const QColor TitleText      = QColor(90, 90, 90);
inline const QColor MutedText      = QColor(120, 120, 120);
inline const QColor GridLine       = QColor(229, 229, 229);

// Status bar.
inline const QColor StatusBarBackground = QColor("#4C3B4D");
inline const QColor StatusBarText       = QColor("#b8b2bd");

}

#endif // THEME_H
