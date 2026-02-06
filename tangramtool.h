#ifndef TANGRAMTOOL_H
#define TANGRAMTOOL_H

#include "basetool.h"

#include <memory>

class TangramGame;
class TangramPiece;
class CanvasWidget;
struct TangramPose;

class TangramTool : public BaseTool
{
public:
    TangramTool(TangramGame* game, CanvasWidget* canvasWidget);

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;
    void drawOverlay(QPainter* painter, QWidget* widget) override;

    QString toolName() const override { return "Tangram"; }

    std::shared_ptr<TangramPiece> currentSelection() const { return selectedPiece; }
    bool rotateSelectionBy(double angleDeg);
    void clearSelection();

private:
    enum class DragMode {
        None,
        Translate,
        Rotate
    };

    void requestCanvasRefresh();
    bool hasOverlapWithOthers(const std::shared_ptr<TangramPiece>& piece) const;

private:
    TangramGame* game;
    CanvasWidget* canvas;

    std::shared_ptr<TangramPiece> activePiece;
    std::shared_ptr<TangramPiece> selectedPiece;
    DragMode dragMode;

    QPoint lastMousePos;
    double startRotationDeg;
    double rotationStartRefDeg;
    bool movedDuringDrag;
};

#endif // TANGRAMTOOL_H
