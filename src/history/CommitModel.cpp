#include "history/CommitModel.h"

#include <QDateTime>
#include <QLocale>

#include <algorithm>

CommitModel::CommitModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int CommitModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int CommitModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant CommitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};

    const HistoryRow &row = m_rows.at(index.row());
    const CommitInfo &commit = row.commit;

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case CommitColumn:
            return commit.subject.isEmpty()
                       ? QStringLiteral("(no commit message)")
                       : commit.subject;
        case AuthorColumn:
            return commit.author;
        case DateColumn:
            return QDateTime::fromSecsSinceEpoch(commit.authorTime)
                .toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        default:
            break;
        }
        break;

    case Qt::ToolTipRole:
        switch (index.column()) {
        case CommitColumn: {
            QString tip = QStringLiteral("<b>%1</b><br/>%2")
                              .arg(commit.subject.toHtmlEscaped(), commit.oid);
            if (!commit.refs.isEmpty()) {
                tip += QStringLiteral("<br/><br/>");
                tip += commit.refs.join(QStringLiteral("<br/>"));
            }
            return tip;
        }
        case AuthorColumn:
            return commit.email.isEmpty()
                       ? commit.author
                       : QStringLiteral("%1 <%2>").arg(commit.author, commit.email);
        case DateColumn:
            return QDateTime::fromSecsSinceEpoch(commit.authorTime)
                .toString(Qt::ISODate);
        default:
            break;
        }
        break;

    case Qt::TextAlignmentRole:
        if (index.column() == DateColumn)
            return QVariant::fromValue(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
        return QVariant::fromValue(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter));

    case GraphRole:
        return QVariant::fromValue(row.graph);

    case RefsRole:
        return commit.refs;

    case ShortHashRole:
        return commit.shortOid;

    case FullOidRole:
        return commit.oid;

    case AuthorEmailRole:
        return commit.email;

    case AuthorTimeRole:
        return commit.authorTime;

    default:
        break;
    }

    return {};
}

QVariant CommitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case CommitColumn:
        return tr("Commit");
    case AuthorColumn:
        return tr("Author");
    case DateColumn:
        return tr("Date");
    default:
        return {};
    }
}

bool CommitModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;
    return m_hasMore;
}

void CommitModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid() || !m_hasMore || m_fetchPending)
        return;

    m_fetchPending = true;
    emit needMore();
}

void CommitModel::appendBatch(const QVector<HistoryRow> &rows, bool hasMore)
{
    m_fetchPending = false;

    if (!rows.isEmpty()) {
        const int first = m_rows.size();
        beginInsertRows(QModelIndex(), first, first + rows.size() - 1);
        m_rows.reserve(m_rows.size() + rows.size());
        for (const HistoryRow &row : rows)
            m_rows.append(row);
        endInsertRows();

        int lanes = m_maxLanes;
        for (const HistoryRow &row : rows)
            lanes = std::max(lanes, row.graph.laneCount);

        if (lanes != m_maxLanes) {
            m_maxLanes = lanes;
            emit maxLanesChanged(lanes);
        }
    }

    m_hasMore = hasMore;

    // Fill the viewport and a small read-ahead window without waiting for the
    // scroll bar to reach the bottom.
    if (m_hasMore && m_rows.size() < kPrefetchRows) {
        m_fetchPending = true;
        emit needMore();
    }
}

void CommitModel::clearRepository()
{
    beginResetModel();
    m_rows.clear();
    m_rows.squeeze();
    m_hasMore = false;
    m_fetchPending = false;
    m_maxLanes = 1;
    endResetModel();

    emit maxLanesChanged(m_maxLanes);
}
