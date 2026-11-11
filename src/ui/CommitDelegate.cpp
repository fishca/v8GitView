#include "ui/CommitDelegate.h"

#include "history/CommitModel.h"
#include "history/GraphRow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionViewItem>

#include <algorithm>
#include <utility>

namespace {

QString displayedRef(const QString &ref)
{
    if (ref.startsWith(QLatin1String("refs/heads/")))
        return ref.mid(11);
    if (ref.startsWith(QLatin1String("refs/remotes/")))
        return ref.mid(13);
    if (ref.startsWith(QLatin1String("refs/tags/")))
        return ref.mid(10);
    return ref;
}

} // namespace

CommitDelegate::CommitDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_monoFont(Theme::monoFont(-1))
    , m_badgeFont(QApplication::font())
{
    m_badgeFont.setPointSizeF(std::max(6.0, m_badgeFont.pointSizeF() - 1.0));
    m_badgeFont.setBold(true);
    setLaneCount(1);
}

void CommitDelegate::setLaneCount(int lanes)
{
    m_laneCount = std::max(1, lanes);
    m_graphWidth = kLeftPad + m_laneCount * kLaneStep + kRightPad;
}

void CommitDelegate::paintBackground(QPainter *painter,
                                     const QStyleOptionViewItem &option) const
{
    const QPalette &palette = option.palette;
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    QColor background = palette.color(QPalette::Base);
    if (option.features & QStyleOptionViewItem::Alternate)
        background = palette.color(QPalette::AlternateBase);

    if (hovered && !selected)
        background = background.lighter(113);
    if (selected)
        background = palette.color(QPalette::Highlight);

    painter->fillRect(option.rect, background);
}

void CommitDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    if (index.column() != CommitModel::CommitColumn) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const bool selected = opt.state & QStyle::State_Selected;
    const QColor textColor = selected
                                 ? opt.palette.color(QPalette::HighlightedText)
                                 : opt.palette.color(QPalette::Text);
    QColor dimColor = textColor;
    dimColor.setAlpha(selected ? 190 : 135);

    painter->save();
    painter->setClipRect(opt.rect);
    painter->setRenderHint(QPainter::Antialiasing, true);

    paintBackground(painter, opt);
    paintGraph(painter, opt, index);

    const int top = opt.rect.top();
    const int height = opt.rect.height();
    const int right = opt.rect.right();
    int x = opt.rect.left() + m_graphWidth;

    // Short hash in a monospace font.
    if (const QString hash = index.data(CommitModel::ShortHashRole).toString();
        !hash.isEmpty()) {
        const QFontMetrics fm(m_monoFont);
        const int hashWidth = fm.horizontalAdvance(hash) + 12;
        painter->setFont(m_monoFont);
        painter->setPen(dimColor);
        painter->drawText(QRect(x, top, hashWidth, height),
                          Qt::AlignLeft | Qt::AlignVCenter, hash);
        x += hashWidth;
    }

    // Branch/tag badges. Leave room for at least a few characters of subject.
    const int subjectMinWidth = std::min(180, std::max(60, right - x));
    paintRefs(painter, opt, index, x, right - subjectMinWidth, textColor);

    const QString subject = index.data(Qt::DisplayRole).toString();
    if (!subject.isEmpty() && x < right) {
        const QFontMetrics fm(opt.font);
        const QRect textRect(x, top, right - x, height);
        const QString elided =
            fm.elidedText(subject, Qt::ElideRight, textRect.width());
        painter->setFont(opt.font);
        painter->setPen(textColor);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elided);
    }

    painter->restore();
}

void CommitDelegate::paintGraph(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    const QVariant value = index.data(CommitModel::GraphRole);
    if (!value.isValid())
        return;

    const GraphRow row = value.value<GraphRow>();
    const int left = option.rect.left();
    const int top = option.rect.top();
    const int bottom = option.rect.bottom();
    const int centerY = option.rect.center().y();

    const auto laneX = [left](int lane) {
        return qreal(left + kLeftPad + lane * kLaneStep);
    };

    painter->setBrush(Qt::NoBrush);
    painter->setRenderHint(QPainter::Antialiasing, true);

    // The lane entering the commit from the row above.
    if (row.incoming) {
        QPen pen(Theme::laneColor(row.color), 2.0);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);
        painter->drawLine(QPointF(laneX(row.lane), top),
                          QPointF(laneX(row.lane), centerY));
    }

    // Passing lanes first, then the edges leaving the commit dot.
    for (const GraphEdge &edge : std::as_const(row.edges)) {
        if (edge.fromLane == row.lane)
            continue;

        QPen pen(Theme::laneColor(edge.color), 2.0);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);

        const QPointF start(laneX(edge.fromLane), top);
        const QPointF end(laneX(edge.toLane), bottom);
        const qreal middleY = (start.y() + end.y()) / 2.0;

        QPainterPath path(start);
        path.cubicTo(start.x(), middleY, end.x(), middleY, end.x(), end.y());
        painter->drawPath(path);
    }

    for (const GraphEdge &edge : std::as_const(row.edges)) {
        if (edge.fromLane != row.lane)
            continue;

        QPen pen(Theme::laneColor(edge.color), 2.0);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);

        const QPointF start(laneX(row.lane), centerY);
        const QPointF end(laneX(edge.toLane), bottom);
        const qreal middleY = (start.y() + end.y()) / 2.0;

        QPainterPath path(start);
        path.cubicTo(start.x(), middleY, end.x(), middleY, end.x(), end.y());
        painter->drawPath(path);
    }

    // Commit dot with a soft halo.
    const QPointF center(laneX(row.lane), centerY);
    const QColor color = Theme::laneColor(row.color);

    QColor halo = color;
    halo.setAlpha(option.state & QStyle::State_Selected ? 120 : 70);
    painter->setPen(Qt::NoPen);
    painter->setBrush(halo);
    painter->drawEllipse(center, 7.0, 7.0);

    painter->setBrush(color);
    painter->drawEllipse(center, 4.3, 4.3);
}

void CommitDelegate::paintRefs(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index, int &x, int rightLimit,
                               const QColor &textColor) const
{
    Q_UNUSED(textColor)

    const QStringList refs = index.data(CommitModel::RefsRole).toStringList();
    if (refs.isEmpty() || x >= rightLimit)
        return;

    const QFontMetrics fm(m_badgeFont);
    const int top = option.rect.top() + (option.rect.height() - 18) / 2;

    int drawn = 0;
    for (int i = 0; i < refs.size(); ++i) {
        const QString &ref = refs.at(i);
        const QString label = displayedRef(ref);
        const int width = fm.horizontalAdvance(label) + 12;

        const bool last = (i == refs.size() - 1);
        const int reserve = last ? 0 : fm.horizontalAdvance(QStringLiteral("+99")) + 14;
        if (x + width + reserve > rightLimit)
            break;

        const QColor color = Theme::refColor(ref);
        QColor background = color;
        background.setAlpha(45);
        QColor border = color;
        border.setAlpha(150);

        const QRectF badge(x, top, width, 18);
        painter->setPen(QPen(border, 1.0));
        painter->setBrush(background);
        painter->drawRoundedRect(badge, 4.0, 4.0);

        painter->setFont(m_badgeFont);
        painter->setPen(color.lighter(145));
        painter->drawText(badge, Qt::AlignCenter, label);

        x += width + 4;
        ++drawn;
    }

    const int remaining = refs.size() - drawn;
    if (remaining > 0) {
        const QString label = QStringLiteral("+%1").arg(remaining);
        const int width = fm.horizontalAdvance(label) + 12;
        if (x + width <= rightLimit) {
            const QRectF badge(x, top, width, 18);
            painter->setPen(QPen(option.palette.color(QPalette::Mid), 1.0));
            painter->setBrush(option.palette.color(QPalette::AlternateBase));
            painter->drawRoundedRect(badge, 4.0, 4.0);
            painter->setFont(m_badgeFont);
            painter->setPen(option.palette.color(QPalette::PlaceholderText));
            painter->drawText(badge, Qt::AlignCenter, label);
            x += width + 4;
        }
    }
}

QSize CommitDelegate::sizeHint(const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(kRowHeight);
    return size;
}
