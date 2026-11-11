#pragma once

#include <QColor>
#include <QFont>
#include <QStyledItemDelegate>

class CommitDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CommitDelegate(QObject *parent = nullptr);

    void setLaneCount(int lanes);
    int laneCount() const { return m_laneCount; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option) const;
    void paintGraph(QPainter *painter, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void paintRefs(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index, int &x, int rightLimit,
                   const QColor &textColor) const;

    int m_laneCount = 1;
    int m_graphWidth = 40;
    QFont m_monoFont;
    QFont m_badgeFont;

    static constexpr int kLaneStep = 16;
    static constexpr int kLeftPad = 10;
    static constexpr int kRightPad = 8;
    static constexpr int kRowHeight = 28;
};
