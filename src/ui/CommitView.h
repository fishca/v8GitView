#pragma once

#include <QMetaObject>
#include <QTableView>

class CommitDelegate;

class CommitView : public QTableView
{
    Q_OBJECT

public:
    explicit CommitView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    CommitDelegate *commitDelegate() const { return m_delegate; }

private:
    CommitDelegate *m_delegate = nullptr;
    QMetaObject::Connection m_lanesConnection;
};
