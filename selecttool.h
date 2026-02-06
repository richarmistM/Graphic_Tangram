#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "basetool.h"
#include <vector>
#include <memory>

class Shape;
class DrawEngine;

class SelectTool : public BaseTool
{
public:
    SelectTool();
    ~SelectTool() override = default;

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

    // Apply a QTransform to all selected shapes
    void applyTransformToSelection(const QTransform& t, DrawEngine* engine);
    void applyTransformToSelection_params(double tx, double ty,
                                                      double sx, double sy,
                                                      double angleDeg,
                                                      const QPointF &ref,
                                          DrawEngine* engine);

    // Set reference point for transforms (used when user picks a custom point)
    void setReferencePoint(const QPointF& p) { referencePoint = p; useCustomRef = true; }

    // Clear selection
    void clearSelection() { selectedShapes.clear(); }

    // overlay draw
    void drawOverlay(QPainter* painter, QWidget* widget) override;

    // get currently selected shapes (read-only)
    const std::vector<std::shared_ptr<Shape>>& getSelection() const { return selectedShapes; }


    void startPickRefMode() { pickRefMode = true; hasPickedRef = false; pickedRefVertexIndex = -1; pickedRefShape.reset(); }
    void cancelPickRefMode() { pickRefMode = false; hasPickedRef = false; pickedRefVertexIndex = -1; pickedRefShape.reset(); }
    QPointF getPickedRefPoint() const { return pickedRefPoint; }
    bool isRefPicked() const { return hasPickedRef; }


    // 新成员
    bool pickRefMode = false;                // 是否处于“Pick Ref”模式
    QPointF pickedRefPoint;                  // 当前自定义参考点坐标（如果有）
    bool hasPickedRef = false;               // 标志：是否已经 pick
    // 记录 pickedRef 是否对应某个 shape 的顶点
    std::weak_ptr<Shape> pickedRefShape;
    int pickedRefVertexIndex = -1;           // 若不是顶点则为 -1
    int pickedRefSearchRadius = 8;           // 查找最近顶点阈值（像素）


private:
    bool isDragging;
    QPoint dragStart;
    QPoint dragEnd;

    std::vector<std::shared_ptr<Shape>> selectedShapes;
    QPointF referencePoint;
    bool useCustomRef = false;
};

#endif // SELECTTOOL_H
