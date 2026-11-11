#pragma once

#include "git/GitRepository.h"
#include "history/GraphLayout.h"
#include "history/HistoryRow.h"

#include <QAtomicInt>
#include <QHash>
#include <QObject>
#include <QStringList>

struct git_oid;
struct git_revwalk;

// Lives in a background thread and walks the history in small batches.
// Requests are queued; cancel() is thread-safe and interrupts the walk
// between commits.
class HistoryLoader : public QObject
{
    Q_OBJECT

public:
    explicit HistoryLoader(QObject *parent = nullptr);
    ~HistoryLoader() override;

    void cancel();

public slots:
    void openRepository(const QString &path);
    void requestMore();
    void shutdown();

signals:
    void opened(const QString &workdir);
    void batch(const QVector<HistoryRow> &rows, bool hasMore);
    void failed(const QString &error);

private:
    void closeWalk();
    void loadNextBatch();
    CommitInfo readCommit(const git_oid &oid) const;

    static constexpr int kBatchSize = 1000;

    GitRepository m_repo;
    git_revwalk *m_walk = nullptr;
    GraphLayout m_layout;
    QHash<QString, QStringList> m_refs;
    QAtomicInt m_cancel;
    bool m_hasMore = false;
};
