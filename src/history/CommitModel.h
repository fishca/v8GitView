#pragma once

#include "history/HistoryRow.h"

#include <QAbstractTableModel>
#include <QVector>

class CommitModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        CommitColumn = 0,
        AuthorColumn,
        DateColumn,
        ColumnCount
    };

    enum Roles
    {
        GraphRole = Qt::UserRole + 1,
        RefsRole,
        ShortHashRole,
        FullOidRole,
        AuthorEmailRole,
        AuthorTimeRole
    };

    explicit CommitModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    void appendBatch(const QVector<HistoryRow> &rows, bool hasMore);
    void clearRepository();

    bool hasMore() const { return m_hasMore; }
    int maxLaneCount() const { return m_maxLanes; }

signals:
    void needMore();
    void maxLanesChanged(int lanes);

private:
    static constexpr int kPrefetchRows = 400;

    QVector<HistoryRow> m_rows;
    bool m_hasMore = false;
    bool m_fetchPending = false;
    int m_maxLanes = 1;
};
