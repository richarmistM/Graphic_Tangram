#include "tangrampiece.h"

#include <QtMath>

namespace {
// helper to rotate a point (x, y) by angle degrees CCW
inline QPointF rotatePoint(const QPointF& p, double angleDeg)
{
    double rad = qDegreesToRadians(angleDeg);
    double c = std::cos(rad);
    double s = std::sin(rad);
    return QPointF(p.x() * c - p.y() * s,
                   p.x() * s + p.y() * c);
}
}

TangramPiece::TangramPiece(TangramPieceType t, const std::vector<QPointF>& baseVerts)
    : type(t),
      baseVertices(baseVerts),
      position(0.0, 0.0),
      rotationDeg(0.0),
      flipped(false)
{
    baseCentroid = computePolygonCentroid(baseVertices);
    rebuildVertices();

    // 默认填充颜色、边颜色
    filled = true;
    color = Qt::black;

    switch (type) {
    case TangramPieceType::LargeA: fillColor = QColor("#ffb347"); break;       // warm orange
    case TangramPieceType::LargeB: fillColor = QColor("#ffcc5c"); break;       // soft yellow
    case TangramPieceType::Medium: fillColor = QColor("#88d8b0"); break;       // mint
    case TangramPieceType::Square: fillColor = QColor("#96ceb4"); break;       // teal
    case TangramPieceType::SmallA: fillColor = QColor("#6c5b7b"); break;       // purple
    case TangramPieceType::SmallB: fillColor = QColor("#c06c84"); break;       // rose
    case TangramPieceType::Parallelogram: fillColor = QColor("#f67280"); break;// pink
    }
}

void TangramPiece::setPose(const TangramPose& pose)
{
    position = pose.position;
    rotationDeg = pose.rotationDeg;
    flipped = pose.flipped;
    rebuildVertices();
}

void TangramPiece::setPosition(const QPointF& pos)
{
    position = pos;
    rebuildVertices();
}

void TangramPiece::translateBy(const QPointF& delta)
{
    position += delta;
    rebuildVertices();
}

void TangramPiece::setRotation(double angleDeg)
{
    rotationDeg = angleDeg;
    rebuildVertices();
}

void TangramPiece::rotateBy(double deltaDeg)
{
    rotationDeg += deltaDeg;
    rebuildVertices();
}

void TangramPiece::setFlipped(bool value)
{
    flipped = value;
    rebuildVertices();
}

void TangramPiece::rebuildVertices()
{
    vertices.clear();
    vertices.reserve(static_cast<int>(baseVertices.size()));

    for (const auto& v : baseVertices)
    {
        QPointF local = v - baseCentroid;
        if (flipped)
            local.setX(-local.x());

        QPointF rotated = rotatePoint(local, rotationDeg);
        QPointF world = rotated + position;
        vertices.emplace_back(qRound(world.x()), qRound(world.y()));
    }
}

QPointF TangramPiece::computePolygonCentroid(const std::vector<QPointF>& pts) const
{
    int n = static_cast<int>(pts.size());
    if (n == 0) return QPointF(0.0, 0.0);

    double area = 0.0;
    double cx = 0.0;
    double cy = 0.0;

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        double xi = pts[i].x();
        double yi = pts[i].y();
        double xj = pts[j].x();
        double yj = pts[j].y();
        double cross = xj * yi - xi * yj;
        area += cross;
        cx += (xj + xi) * cross;
        cy += (yj + yi) * cross;
    }

    area *= 0.5;
    if (std::abs(area) < 1e-6) {
        double sx = 0.0, sy = 0.0;
        for (const auto& p : pts) {
            sx += p.x();
            sy += p.y();
        }
        return QPointF(sx / n, sy / n);
    }

    cx /= (6.0 * area);
    cy /= (6.0 * area);
    return QPointF(cx, cy);
}

