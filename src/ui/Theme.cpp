#include "ui/Theme.h"

#include <QApplication>
#include <QFontInfo>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>

namespace {

const QColor kLaneColors[] = {
    QColor("#4E9CF5"), QColor("#F28E2B"), QColor("#59A14F"), QColor("#E15759"),
    QColor("#B07AA1"), QColor("#76B7B2"), QColor("#EDC948"), QColor("#FF9DA7"),
    QColor("#9C755F"), QColor("#BAB0AC"), QColor("#5B8FF9"), QColor("#61DDAA"),
    QColor("#65789B"), QColor("#F6BD16"), QColor("#7262FD"), QColor("#78D3F8"),
};

constexpr int kLaneColorCount = int(sizeof(kLaneColors) / sizeof(kLaneColors[0]));

} // namespace

namespace Theme {

void applyDarkPalette(QApplication &app)
{
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
        app.setStyle(fusion);

    QPalette p;
    p.setColor(QPalette::Window, QColor("#1E1F22"));
    p.setColor(QPalette::WindowText, QColor("#DFE1E5"));
    p.setColor(QPalette::Base, QColor("#2B2D30"));
    p.setColor(QPalette::AlternateBase, QColor("#2F3136"));
    p.setColor(QPalette::ToolTipBase, QColor("#2B2D30"));
    p.setColor(QPalette::ToolTipText, QColor("#DFE1E5"));
    p.setColor(QPalette::Text, QColor("#DFE1E5"));
    p.setColor(QPalette::Button, QColor("#2B2D30"));
    p.setColor(QPalette::ButtonText, QColor("#DFE1E5"));
    p.setColor(QPalette::BrightText, QColor("#FF6B68"));
    p.setColor(QPalette::Highlight, QColor("#3574F0"));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link, QColor("#548AF7"));
    p.setColor(QPalette::PlaceholderText, QColor("#7A7E85"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#6F737A"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#6F737A"));
    app.setPalette(p);
}

int laneColorCount()
{
    return kLaneColorCount;
}

QIcon appIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(QColor("#3A3D41"), 2.0));
    painter.setBrush(QColor("#1E1F22"));
    painter.drawRoundedRect(QRectF(1.0, 1.0, 62.0, 62.0), 14.0, 14.0);

    const QColor blue = laneColor(0);
    const QColor orange = laneColor(1);
    const QColor green = laneColor(2);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(blue, 5.0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(24.0, 12.0), QPointF(24.0, 52.0));

    painter.setPen(QPen(orange, 5.0, Qt::SolidLine, Qt::RoundCap));
    QPainterPath branchUp(QPointF(24.0, 17.0));
    branchUp.cubicTo(36.0, 17.0, 41.0, 20.0, 44.0, 24.0);
    painter.drawPath(branchUp);

    painter.setPen(QPen(green, 5.0, Qt::SolidLine, Qt::RoundCap));
    QPainterPath branchDown(QPointF(24.0, 47.0));
    branchDown.cubicTo(36.0, 47.0, 41.0, 44.0, 44.0, 40.0);
    painter.drawPath(branchDown);

    painter.setPen(QPen(QColor("#1E1F22"), 2.0));
    painter.setBrush(blue);
    painter.drawEllipse(QPointF(24.0, 14.0), 5.2, 5.2);
    painter.drawEllipse(QPointF(24.0, 50.0), 5.2, 5.2);
    painter.setBrush(orange);
    painter.drawEllipse(QPointF(44.0, 24.0), 5.2, 5.2);
    painter.setBrush(green);
    painter.drawEllipse(QPointF(44.0, 40.0), 5.2, 5.2);

    return QIcon(pixmap);
}

QColor laneColor(int index)
{
    if (index < 0)
        index = 0;
    return kLaneColors[index % kLaneColorCount];
}

QColor refColor(const QString &ref)
{
    if (ref.startsWith(QLatin1String("refs/remotes/")))
        return QColor("#CC7832");
    if (ref.startsWith(QLatin1String("refs/tags/")))
        return QColor("#9876AA");
    if (ref.startsWith(QLatin1String("refs/heads/")))
        return QColor("#3D7F3D");
    return QColor("#4B6EAF");
}

QFont monoFont(int pointSizeDelta)
{
    QFont f(QStringLiteral("Cascadia Mono"));
    if (!QFontInfo(f).family().contains(QLatin1String("Cascadia"), Qt::CaseInsensitive))
        f.setFamily(QStringLiteral("Consolas"));
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(f.pointSize() + pointSizeDelta);
    return f;
}

} // namespace Theme
