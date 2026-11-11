#include "ui/CommitView.h"

#include "history/CommitModel.h"
#include "ui/CommitDelegate.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QScrollBar>

CommitView::CommitView(QWidget *parent)
    : QTableView(parent)
    , m_delegate(new CommitDelegate(this))
{
    setObjectName(QStringLiteral("commitView"));
    setItemDelegate(m_delegate);

    setShowGrid(false);
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWordWrap(false);
    setTextElideMode(Qt::ElideRight);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
    setCornerButtonEnabled(false);
    setMouseTracking(true);

    verticalHeader()->hide();
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(28);
    verticalHeader()->setMinimumSectionSize(28);

    QHeaderView *header = horizontalHeader();
    header->setHighlightSections(false);
    header->setSectionsClickable(false);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(60);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setStyleSheet(QStringLiteral(
        "QTableView { border: none; }"
        "QHeaderView::section {"
        "  background: #2B2D30;"
        "  color: #9DA0A8;"
        "  border: none;"
        "  border-bottom: 1px solid #3A3D41;"
        "  padding: 5px 8px;"
        "  font-weight: 600;"
        "}"
        "QScrollBar:vertical {"
        "  background: #24262A;"
        "  width: 12px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #4A4E54;"
        "  min-height: 28px;"
        "  border-radius: 4px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #5C6168; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal {"
        "  background: #24262A;"
        "  height: 12px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #4A4E54;"
        "  min-width: 28px;"
        "  border-radius: 4px;"
        "  margin: 2px;"
        "}"
        "QScrollBar::handle:horizontal:hover { background: #5C6168; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"));
}

void CommitView::setModel(QAbstractItemModel *model)
{
    if (m_lanesConnection)
        disconnect(m_lanesConnection);

    QTableView::setModel(model);

    // Modes must be applied after the model has been installed: QHeaderView
    // ignores them while it still has zero sections.
    QHeaderView *header = horizontalHeader();
    header->setSectionResizeMode(CommitModel::CommitColumn, QHeaderView::Stretch);
    header->setSectionResizeMode(CommitModel::AuthorColumn, QHeaderView::Fixed);
    header->setSectionResizeMode(CommitModel::DateColumn, QHeaderView::Fixed);
    header->resizeSection(CommitModel::AuthorColumn, 190);
    header->resizeSection(CommitModel::DateColumn, 150);

    if (auto *commitModel = qobject_cast<CommitModel *>(model)) {
        m_lanesConnection =
            connect(commitModel, &CommitModel::maxLanesChanged, m_delegate,
                    &CommitDelegate::setLaneCount);
        m_delegate->setLaneCount(commitModel->maxLaneCount());
    }
}
