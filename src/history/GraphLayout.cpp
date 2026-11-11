#include "history/GraphLayout.h"

#include "ui/Theme.h"

#include <algorithm>
#include <utility>

void GraphLayout::reset()
{
    m_lanes.clear();
    m_nextColor = 0;
}

int GraphLayout::allocColor()
{
    const int count = std::max(1, Theme::laneColorCount());
    const int color = m_nextColor % count;
    ++m_nextColor;
    return color;
}

int GraphLayout::findLane(const QString &oid) const
{
    for (int i = 0; i < m_lanes.size(); ++i) {
        if (m_lanes[i].oid == oid)
            return i;
    }
    return -1;
}

GraphRow GraphLayout::next(const CommitInfo &commit)
{
    int commitLane = findLane(commit.oid);
    bool incoming = commitLane >= 0;

    if (commitLane < 0) {
        // A head commit: start a new lane on the right.
        m_lanes.append({commit.oid, allocColor()});
        commitLane = m_lanes.size() - 1;
    }

    const QVector<Lane> top = m_lanes;
    const int commitColor = top[commitLane].color;

    GraphRow row;
    row.lane = commitLane;
    row.color = commitColor;
    row.incoming = incoming;

    // Everything below the consumed lane shifts up by one.
    m_lanes.removeAt(commitLane);

    // Parents: reuse a lane that already waits for the same commit, otherwise
    // open a new lane. The first parent continues in the commit's own column.
    const int parentCount = int(commit.parents.size());
    for (int i = 0; i < parentCount; ++i) {
        const QString &parent = commit.parents[i];

        int target = findLane(parent);
        if (target >= 0) {
            row.edges.append({commitLane, target, m_lanes[target].color});
            continue;
        }

        int insertAt = 0;
        int color = 0;
        if (i == 0) {
            insertAt = std::min(commitLane, int(m_lanes.size()));
            color = commitColor;
        } else {
            insertAt = m_lanes.size();
            color = allocColor();
        }

        m_lanes.insert(insertAt, {parent, color});
        row.edges.append({commitLane, insertAt, color});
    }

    // Lanes that pass through this row. Their oid is unique, so it identifies
    // the lane after the insertions above.
    for (int k = 0; k < top.size(); ++k) {
        if (k == commitLane)
            continue;

        const int to = findLane(top[k].oid);
        if (to >= 0)
            row.edges.append({k, to, top[k].color});
    }

    row.laneCount = std::max(1, std::max(int(top.size()), int(m_lanes.size())));
    for (const GraphEdge &edge : std::as_const(row.edges)) {
        row.laneCount = std::max(row.laneCount,
                                 std::max(edge.fromLane, edge.toLane) + 1);
    }

    return row;
}
