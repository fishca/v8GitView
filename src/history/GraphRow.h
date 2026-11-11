#pragma once

#include <QMetaType>
#include <QVector>

struct GraphEdge
{
    int fromLane = 0;
    int toLane = 0;
    int color = 0;
};

struct GraphRow
{
    int lane = 0;
    int color = 0;
    int laneCount = 1;
    bool incoming = false; // the lane enters this row from above
    QVector<GraphEdge> edges;
};

Q_DECLARE_METATYPE(GraphEdge)
Q_DECLARE_METATYPE(GraphRow)
