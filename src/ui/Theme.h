#pragma once

#include <QColor>
#include <QFont>
#include <QIcon>

class QApplication;

namespace Theme {

void applyDarkPalette(QApplication &app);

QIcon appIcon();

int laneColorCount();
QColor laneColor(int index);

// Badge colour for a ref: local branch, remote branch, tag or other.
QColor refColor(const QString &ref);

QFont monoFont(int pointSizeDelta = 0);

} // namespace Theme
