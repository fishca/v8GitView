#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

struct git_repository;

// Thin RAII wrapper around libgit2. Not thread-safe: use one instance per thread.
class GitRepository
{
public:
    GitRepository();
    ~GitRepository();

    GitRepository(const GitRepository &) = delete;
    GitRepository &operator=(const GitRepository &) = delete;

    static void globalInit();
    static void globalShutdown();

    bool open(const QString &path, QString *error = nullptr);
    void close();

    bool isValid() const { return m_repo != nullptr; }
    QString workdir() const;

    // Commit oid (hex) -> list of refs pointing at it ("refs/heads/main", ...).
    QHash<QString, QStringList> refs() const;

    git_repository *handle() const { return m_repo; }

private:
    git_repository *m_repo = nullptr;
};
