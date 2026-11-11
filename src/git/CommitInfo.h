#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>

struct CommitInfo
{
    QString oid;
    QString shortOid;
    QStringList parents;
    QString author;
    QString email;
    qint64 authorTime = 0;
    QString subject;
    QStringList refs;
};

Q_DECLARE_METATYPE(CommitInfo)
