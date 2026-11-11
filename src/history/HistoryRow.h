#pragma once

#include "git/CommitInfo.h"
#include "history/GraphRow.h"

#include <QMetaType>
#include <QVector>

struct HistoryRow
{
    CommitInfo commit;
    GraphRow graph;
};

Q_DECLARE_METATYPE(HistoryRow)
