#include "history/HistoryLoader.h"

#include <git2.h>

#include <QVector>

#include <utility>

namespace {

QString lastError()
{
    const git_error *err = git_error_last();
    return err && err->message ? QString::fromUtf8(err->message)
                               : QStringLiteral("unknown libgit2 error");
}

QString oidToString(const git_oid *oid)
{
    char buf[GIT_OID_HEXSZ + 1] = {};
    git_oid_tostr(buf, sizeof(buf), oid);
    return QString::fromLatin1(buf);
}

} // namespace

HistoryLoader::HistoryLoader(QObject *parent)
    : QObject(parent)
{
}

HistoryLoader::~HistoryLoader()
{
    closeWalk();
}

void HistoryLoader::cancel()
{
    m_cancel.storeRelaxed(1);
}

void HistoryLoader::closeWalk()
{
    if (m_walk) {
        git_revwalk_free(m_walk);
        m_walk = nullptr;
    }
    m_hasMore = false;
}

void HistoryLoader::openRepository(const QString &path)
{
    m_cancel.storeRelaxed(0);
    closeWalk();
    m_repo.close();
    m_refs.clear();
    m_layout.reset();

    QString error;
    if (!m_repo.open(path, &error)) {
        emit failed(error);
        return;
    }

    if (git_repository_is_empty(m_repo.handle())) {
        emit opened(m_repo.workdir());
        emit batch({}, false);
        return;
    }

    m_refs = m_repo.refs();

    if (git_revwalk_new(&m_walk, m_repo.handle()) != 0) {
        emit failed(lastError());
        return;
    }

    git_revwalk_sorting(m_walk, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);

    if (git_revwalk_push_head(m_walk) != 0) {
        emit failed(lastError());
        closeWalk();
        return;
    }

    emit opened(m_repo.workdir());
    loadNextBatch();
}

void HistoryLoader::requestMore()
{
    if (!m_walk || !m_hasMore)
        return;
    loadNextBatch();
}

void HistoryLoader::loadNextBatch()
{
    QVector<HistoryRow> rows;
    rows.reserve(kBatchSize);

    bool exhausted = false;
    while (rows.size() < kBatchSize) {
        if (m_cancel.loadRelaxed() != 0)
            return;

        git_oid oid;
        if (git_revwalk_next(&oid, m_walk) != 0) {
            exhausted = true;
            break;
        }

        HistoryRow row;
        row.commit = readCommit(oid);
        row.graph = m_layout.next(row.commit);
        rows.push_back(std::move(row));
    }

    m_hasMore = !exhausted;

    if (m_cancel.loadRelaxed() != 0)
        return;

    emit batch(rows, m_hasMore);
}

CommitInfo HistoryLoader::readCommit(const git_oid &oid) const
{
    CommitInfo info;
    info.oid = oidToString(&oid);
    info.shortOid = info.oid.left(8);

    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, m_repo.handle(), &oid) != 0) {
        info.subject = QStringLiteral("(unreadable commit)");
        return info;
    }

    const unsigned int parentCount = git_commit_parentcount(commit);
    info.parents.reserve(int(parentCount));
    for (unsigned int i = 0; i < parentCount; ++i)
        info.parents.append(oidToString(git_commit_parent_id(commit, i)));

    if (const git_signature *author = git_commit_author(commit)) {
        if (author->name)
            info.author = QString::fromUtf8(author->name);
        if (author->email)
            info.email = QString::fromUtf8(author->email);
        info.authorTime = qint64(author->when.time);
    }

    if (const char *summary = git_commit_summary(commit))
        info.subject = QString::fromUtf8(summary);

    info.refs = m_refs.value(info.oid);

    git_commit_free(commit);
    return info;
}

void HistoryLoader::shutdown()
{
    closeWalk();
    m_repo.close();
    m_refs.clear();
    m_layout.reset();
}
