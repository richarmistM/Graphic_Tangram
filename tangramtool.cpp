#include "tangramtool.h"

#include "tangramgame.h"
#include "tangrampiece.h"
#include "canvaswidget.h"
#include "drawengine.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <cmath>
#include <QPolygon>
#include <QPolygonF>

namespace {
constexpr double SNAP_POSITION_THRESHOLD = 28.0;
constexpr double SNAP_ANGLE_THRESHOLD = 12.0;
}

TangramTool::TangramTool(TangramGame* g, CanvasWidget* canvasWidget)
    : game(g),
      canvas(canvasWidget),
      dragMode(DragMode::None),
      startRotationDeg(0.0),
      rotationStartRefDeg(0.0),
      movedDuringDrag(false)
{
}

void TangramTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    Q_UNUSED(engine);
    if (!game || game->isAnimating()) return;

    if (e->button() == Qt::LeftButton)
    {
        auto piece = game->pieceAt(e->pos());
        activePiece = piece;
        selectedPiece = piece;
        movedDuringDrag = false;

        if (piece)
        {
            game->bringToFront(piece);
            lastMousePos = e->pos();

            if (e->modifiers() & Qt::ShiftModifier)
            {
                dragMode = DragMode::Rotate;
                startRotationDeg = piece->pose().rotationDeg;

                QPointF center = piece->pose().position;
                QPointF v = QPointF(lastMousePos) - center;
                rotationStartRefDeg = qRadiansToDegrees(std::atan2(v.y(), v.x()));
            }
            else
            {
                dragMode = DragMode::Translate;
            }
        }
        else
        {
            dragMode = DragMode::None;
            selectedPiece.reset();
            requestCanvasRefresh();
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        auto piece = game->pieceAt(e->pos());
        if (piece && piece->pieceType() == TangramPieceType::Parallelogram)
        {
            bool newFlip = !piece->isFlipped();
            piece->setFlipped(newFlip);
            if (hasOverlapWithOthers(piece)) {
                piece->setFlipped(!newFlip);
            } else {
                game->bringToFront(piece);
                selectedPiece = piece;
                requestCanvasRefresh();
            }
        }
        else
        {
            selectedPiece.reset();
            requestCanvasRefresh();
        }
    }

    requestCanvasRefresh();
}

void TangramTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    Q_UNUSED(engine);
    if (!game || !activePiece) return;
    if (!(e->buttons() & Qt::LeftButton)) return;
    if (game->isAnimating()) return;

    QPoint currentPos = e->pos();

    if (dragMode == DragMode::Translate)
    {
        QPointF delta = QPointF(currentPos - lastMousePos);
        if (!delta.isNull())
        {
            activePiece->translateBy(delta);
            if (hasOverlapWithOthers(activePiece)) {
                activePiece->translateBy(-delta);
            } else {
                lastMousePos = currentPos;
                movedDuringDrag = true;
                requestCanvasRefresh();
            }
        }
    }
    else if (dragMode == DragMode::Rotate)
    {
        QPointF center = activePiece->pose().position;
        QPointF v = QPointF(currentPos) - center;
        if (!qFuzzyIsNull(v.manhattanLength()))
        {
            double currentAngle = qRadiansToDegrees(std::atan2(v.y(), v.x()));
            double deltaAngle = currentAngle - rotationStartRefDeg;
            double newAngle = startRotationDeg + deltaAngle;
            double prevAngle = activePiece->pose().rotationDeg;
            activePiece->setRotation(newAngle);
            if (hasOverlapWithOthers(activePiece)) {
                activePiece->setRotation(prevAngle);
            } else {
                movedDuringDrag = true;
                requestCanvasRefresh();
            }
        }
    }
}

void TangramTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    Q_UNUSED(engine);
    if (!game) return;

    if (e->button() == Qt::LeftButton)
    {
        if (activePiece && movedDuringDrag)
        {
            game->snapPieceToTarget(activePiece,
                                    SNAP_POSITION_THRESHOLD,
                                    SNAP_ANGLE_THRESHOLD);
        }

        activePiece.reset();
        dragMode = DragMode::None;
        movedDuringDrag = false;
    }
    else if (e->button() == Qt::RightButton)
    {
        // no-op
    }
}

void TangramTool::drawOverlay(QPainter* painter, QWidget* widget)
{
    Q_UNUSED(widget);
    if (!painter) return;

    std::shared_ptr<TangramPiece> highlight = activePiece ? activePiece : selectedPiece;
    if (highlight)
    {
        painter->save();
        painter->setPen(QPen(Qt::darkBlue, 2, Qt::DashLine));

        const auto& verts = highlight->vertices;
        if (!verts.empty())
        {
            QPolygon poly;
            for (const auto& v : verts)
                poly << v;
            painter->drawPolygon(poly);
        }

        painter->restore();
    }
}

void TangramTool::requestCanvasRefresh()
{
    if (canvas)
        canvas->update();
}

namespace {

double polygonArea(const QPolygonF& poly)
{
    if (poly.size() < 3)
        return 0.0;

    double sum = 0.0;
    for (int i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
    {
        sum += poly[j].x() * poly[i].y() - poly[i].x() * poly[j].y();
    }
    return std::abs(sum) * 0.5;
}

}

bool TangramTool::hasOverlapWithOthers(const std::shared_ptr<TangramPiece>& piece) const
{
    if (!game || !piece) return false;

    auto buildPoly = [](const TangramPiece& p) {
        QPolygonF poly;
        for (const auto& v : p.vertices)
            poly << QPointF(v);
        return poly;
    };

    QPolygonF subject = buildPoly(*piece);
    for (const auto& other : game->pieces())
    {
        if (!other || other == piece) continue;
        QPolygonF target = buildPoly(*other);
        QPolygonF inter = subject.intersected(target);
        if (inter.isEmpty())
            continue;
        if (polygonArea(inter) > 0.5)
            return true;
    }
    return false;
}

bool TangramTool::rotateSelectionBy(double angleDeg)
{
    if (!selectedPiece || std::abs(angleDeg) < 1e-6)
        return false;

    double previousAngle = selectedPiece->pose().rotationDeg;
    selectedPiece->rotateBy(angleDeg);
    if (hasOverlapWithOthers(selectedPiece))
    {
        selectedPiece->setRotation(previousAngle);
        return false;
    }

    requestCanvasRefresh();
    return true;
}

void TangramTool::clearSelection()
{
    activePiece.reset();
    selectedPiece.reset();
    dragMode = DragMode::None;
    movedDuringDrag = false;
    requestCanvasRefresh();
}
