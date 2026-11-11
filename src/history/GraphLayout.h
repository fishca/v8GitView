#pragma once

#include "git/CommitInfo.h"
#include "history/GraphRow.h"

#include <QString>
#include <QVector>

// Incremental lane layout for a git commit graph.
//
// Commits must be fed in topological order (a parent never precedes its
// child). The layout is computed in O(N * lanes), where the number of
// active lanes is normally well below 20 even for very busy repositories.
class GraphLayout
{
public:
    void reset();

    // Consumes the next commit and returns the row to paint below it.
    GraphRow next(const CommitInfo &commit);

    int activeLaneCount() const { return m_lanes.size(); }

private:
    struct Lane
    {
        QString oid;
        int color = 0;
    };

    int allocColor();
    int findLane(const QString &oid) const;

    QVector<Lane> m_lanes;
    int m_nextColor = 0;
};
