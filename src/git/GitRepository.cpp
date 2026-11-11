#include "git/GitRepository.h"

#include <git2.h>

#include <QDir>
#include <QFileInfo>

#include <algorithm>

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

GitRepository::GitRepository() = default;

GitRepository::~GitRepository()
{
    close();
}

void GitRepository::globalInit()
{
    git_libgit2_init();
}

void GitRepository::globalShutdown()
{
    git_libgit2_shutdown();
}

bool GitRepository::open(const QString &path, QString *error)
{
    close();

    git_repository *repo = nullptr;
    const int rc = git_repository_open_ext(&repo, QDir::toNativeSeparators(path).toUtf8().constData(),
                                           0, nullptr);
    if (rc != 0) {
        if (error)
            *error = lastError();
        return false;
    }

    m_repo = repo;
    return true;
}

void GitRepository::close()
{
    if (m_repo) {
        git_repository_free(m_repo);
        m_repo = nullptr;
    }
}

QString GitRepository::workdir() const
{
    if (!m_repo)
        return {};

    const char *wd = git_repository_workdir(m_repo);
    if (!wd)
        wd = git_repository_path(m_repo);

    QString path = QString::fromUtf8(wd);
    if (path.endsWith(QLatin1Char('/')) || path.endsWith(QLatin1Char('\\')))
        path.chop(1);
    return QDir::toNativeSeparators(path);
}

QHash<QString, QStringList> GitRepository::refs() const
{
    QHash<QString, QStringList> result;
    if (!m_repo)
        return result;

    git_reference_iterator *it = nullptr;
    if (git_reference_iterator_new(&it, m_repo) != 0)
        return result;

    git_reference *ref = nullptr;
    while (git_reference_next(&ref, it) == 0) {
        const char *name = git_reference_name(ref);
        // HEAD and remote HEAD symrefs only duplicate the real branch badges.
        const bool skip = !name || qstrcmp(name, "HEAD") == 0
                          || qstrcmp(name, "refs/remotes/origin/HEAD") == 0;
        if (name && !skip) {
            git_object *obj = nullptr;
            if (git_reference_peel(&obj, ref, GIT_OBJECT_COMMIT) == 0) {
                result[oidToString(git_object_id(obj))].append(QString::fromUtf8(name));
                git_object_free(obj);
            }
        }
        git_reference_free(ref);
        ref = nullptr;
    }

    git_reference_iterator_free(it);

    // Stable order, no duplicates.
    for (auto &list : result) {
        list.sort();
        list.erase(std::unique(list.begin(), list.end()), list.end());
    }
    return result;
}
