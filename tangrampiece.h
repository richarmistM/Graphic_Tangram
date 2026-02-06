#ifndef TANGRAMPIECE_H
#define TANGRAMPIECE_H

#include "polygonshape.h"

#include <QPointF>
#include <vector>

enum class TangramPieceType {
    LargeA = 0,
    LargeB,
    Medium,
    Square,
    SmallA,
    SmallB,
    Parallelogram
};

struct TangramPose {
    QPointF position;
    double rotationDeg;
    bool flipped;
};

class TangramPiece : public PolygonShape
{
public:
    TangramPiece(TangramPieceType t, const std::vector<QPointF>& baseVerts);

    TangramPieceType pieceType() const { return type; }

    void setPose(const TangramPose& pose);
    TangramPose pose() const { return {position, rotationDeg, flipped}; }

    void setPosition(const QPointF& pos);
    void translateBy(const QPointF& delta);

    void setRotation(double angleDeg);
    void rotateBy(double deltaDeg);

    void setFlipped(bool value);
    bool isFlipped() const { return flipped; }

    QPointF currentCentroid() const { return position; }

private:
    void rebuildVertices();
    QPointF computePolygonCentroid(const std::vector<QPointF>& pts) const;

private:
    TangramPieceType type;
    std::vector<QPointF> baseVertices;
    QPointF baseCentroid;

    QPointF position;   // world-space centroid
    double rotationDeg; // CCW
    bool flipped;
};

#endif // TANGRAMPIECE_H

