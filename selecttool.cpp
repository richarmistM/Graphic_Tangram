#include "selecttool.h"
#include "drawengine.h"
#include "shape.h"
#include "lineshape.h"
#include "arcshape.h"
#include "polygonshape.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>

static QPointF transform_point_about_ref(const QPointF &p,
                                         const QPointF &ref,
                                         double sx, double sy,
                                         double angleDeg,
                                         double tx, double ty)
{
    double x = p.x() - ref.x();
    double y = p.y() - ref.y();

    x *= sx;
    y *= sy;

    double a = angleDeg * M_PI / 180.0;
    double cosA = std::cos(a);
    double sinA = std::sin(a);
    double xr = x * cosA - y * sinA;
    double yr = x * sinA + y * cosA;

    double xf = xr + ref.x() + tx;
    double yf = yr + ref.y() + ty;

    return QPointF(xf, yf);
}


SelectTool::SelectTool()
    : isDragging(false), useCustomRef(false)
{
}

void SelectTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    // 如果处于 pickRefMode，则优先处理参考点拾取（single click）

    if (pickRefMode && e->button() == Qt::LeftButton)
    {
        QPoint clicked = e->pos();
        hasPickedRef = false;
        pickedRefVertexIndex = -1;
        pickedRefShape.reset();

        // 1) 在当前选中图元中查找最近顶点（按像素距离阈值）
        const int TH = pickedRefSearchRadius;
        double bestDist2 = (TH+1)*(TH+1);
        std::shared_ptr<Shape> bestShape = nullptr;
        int bestIndex = -1;

        for (auto &sp : selectedShapes)
        {
            // 只对具有顶点的 shape 做查找：LineShape (2 pts), PolygonShape (vertices), BezierShape (controlPoints)
            // 需要 dynamic cast 检查具体类型
            if (auto line = std::dynamic_pointer_cast<LineShape>(sp)) {
                QPoint vs[2] = { line->start, line->end };
                for (int i=0;i<2;++i) {
                    double dx = vs[i].x() - clicked.x();
                    double dy = vs[i].y() - clicked.y();
                    double d2 = dx*dx + dy*dy;
                    if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = i; }
                }
            } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp)) {
                for (int i=0;i<(int)poly->vertices.size();++i) {
                    QPoint v = poly->vertices[i];
                    double dx = v.x() - clicked.x(), dy = v.y() - clicked.y();
                    double d2 = dx*dx + dy*dy;
                    if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = i; }
                }
            } /*else if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp)) {
                for (int i=0;i<(int)bez->controlPoints.size();++i) {
                    QPointF v = bez->controlPoints[i];
                    double dx = v.x() - clicked.x(), dy = v.y() - clicked.y();
                    double d2 = dx*dx + dy*dy;
                    if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = i; }
                }
            }*/
            // ArcShape: 可以检测圆心是否接近
            else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp)) {
                double dx = arc->center.x() - clicked.x(), dy = arc->center.y() - clicked.y();
                double d2 = dx*dx + dy*dy;
                if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = 0; } // vertexIndex 0 表示圆心
            }
        }

        if (bestShape) {
            // pick 成功，记录为顶点引用
            pickedRefShape = bestShape;
            pickedRefVertexIndex = bestIndex;

            if (auto line = std::dynamic_pointer_cast<LineShape>(bestShape)) {
                pickedRefPoint = (bestIndex==0) ? QPointF(line->start) : QPointF(line->end);
            } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(bestShape)) {
                pickedRefPoint = QPointF(poly->vertices[bestIndex]);
            } /*else if (auto bez = std::dynamic_pointer_cast<BezierShape>(bestShape)) {
                pickedRefPoint = bez->controlPoints[bestIndex];
            }*/ else if (auto arc = std::dynamic_pointer_cast<ArcShape>(bestShape)) {
                pickedRefPoint = QPointF(arc->center);
            }
        } else {
            // 没找到顶点，直接把点击点当作 canvas point
            pickedRefShape.reset();
            pickedRefVertexIndex = -1;
            pickedRefPoint = QPointF(clicked);
        }

        hasPickedRef = true;
        pickRefMode = false; // 退出 pick 模式（一次性选择）

        return;
    }

    if (e->button() == Qt::LeftButton)
    {
        // 单击选择：若无移动则视为单选；如果按住拖动则为框选
        isDragging = true;
        dragStart = e->pos();
        dragEnd = dragStart;
    }
}

void SelectTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    if (!isDragging) return;

    dragEnd = e->pos();
    // overlay 会在 CanvasWidget::paintEvent 调用 drawOverlay，显示拖拽矩形
    engine->redrawShape(nullptr);
}

void SelectTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    if (!isDragging) return;
    isDragging = false;
    dragEnd = e->pos();

    // 如果移动距离很小，视为单击
    int dx = std::abs(dragEnd.x() - dragStart.x());
    int dy = std::abs(dragEnd.y() - dragStart.y());
    const int CLICK_THRESH = 4;

    selectedShapes.clear();

    auto shapes = engine->getShapes();
    if (dx <= CLICK_THRESH && dy <= CLICK_THRESH)
    {
        // 单击：从后向前选（顶层优先）
        for (auto it = shapes.rbegin(); it != shapes.rend(); ++it)
        {
            if ((*it)->contains(dragEnd))
            {
                selectedShapes.push_back(*it);
                break;
            }
        }
    }
    else
    {
        // 框选：选择所有和矩形有交集或中心在矩形内的 shape
        int xmin = std::min(dragStart.x(), dragEnd.x());
        int xmax = std::max(dragStart.x(), dragEnd.x());
        int ymin = std::min(dragStart.y(), dragEnd.y());
        int ymax = std::max(dragStart.y(), dragEnd.y());

        for (auto &sp : shapes)
        {
            QPointF c = sp->centroid();
            if (c.x() >= xmin && c.x() <= xmax && c.y() >= ymin && c.y() <= ymax)
                selectedShapes.push_back(sp);
        }
    }

    // 如果用户没有设置自定义参考点，默认重心为变换中心
    useCustomRef = false;
}

void SelectTool::applyTransformToSelection_params(double tx, double ty,
                                                  double sx, double sy,
                                                  double angleDeg,
                                                  const QPointF &ref,
                                                  DrawEngine* engine)
{
    if (!engine) return;

    // 遍历所有被选图形并对其几何顶点做逐点变换
    for (auto &sp : selectedShapes)
    {
        // LineShape
        if (auto line = std::dynamic_pointer_cast<LineShape>(sp))
        {
            QPointF s = transform_point_about_ref(QPointF(line->start), ref, sx, sy, angleDeg, tx, ty);
            QPointF e = transform_point_about_ref(QPointF(line->end),   ref, sx, sy, angleDeg, tx, ty);

            line->start = QPoint(int(std::round(s.x())), int(std::round(s.y())));
            line->end   = QPoint(int(std::round(e.x())), int(std::round(e.y())));

            engine->redrawShape(line);
            continue;
        }

        // PolygonShape
        if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp))
        {
            for (auto &pt : poly->vertices)
            {
                QPointF qf = transform_point_about_ref(QPointF(pt), ref, sx, sy, angleDeg, tx, ty);
                pt = QPoint(int(std::round(qf.x())), int(std::round(qf.y())));
            }
            engine->redrawShape(poly);
            continue;
        }

        // ArcShape
        if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp))
        {
            QPointF newCenter = transform_point_about_ref(QPointF(arc->center), ref, sx, sy, angleDeg, tx, ty);

            double scaleApprox = (std::abs(sx) + std::abs(sy)) * 0.5;
            double newR = arc->radius * scaleApprox;

            arc->center = QPoint(int(std::round(newCenter.x())), int(std::round(newCenter.y())));
            arc->radius = std::max(0, int(std::round(newR)));

            arc->startAngle += angleDeg;
            arc->endAngle   += angleDeg;

            if (arc->startAngle >= 360.0) arc->startAngle = fmod(arc->startAngle, 360.0);
            if (arc->endAngle >= 360.0)   arc->endAngle   = fmod(arc->endAngle,   360.0);

            engine->redrawShape(arc);
            continue;
        }

        // BezierShape
        // if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp))
        // {
        //     for (auto &cp : bez->controlPoints)
        //     {
        //         QPointF qf = transform_point_about_ref(cp, ref, sx, sy, angleDeg, tx, ty);
        //         cp = QPointF(qf.x(), qf.y());
        //     }
        //     engine->redrawShape(bez);
        //     continue;
        // }
    }
}


void SelectTool::drawOverlay(QPainter* painter, QWidget* widget)
{
    Q_UNUSED(widget);
    // 绘制拖拽矩形（如果在拖拽中）
    if (isDragging)
    {
        QRect r(dragStart, dragEnd);
        painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);
    }

    // 绘制已选中图元的边框
    painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
    for (auto &sp : selectedShapes)
    {
        QPointF c = sp->centroid();
        QRectF bbox(c.x()-6, c.y()-6, 12, 12);
        painter->drawRect(bbox);
    }

    // 如果用户设置了自定义参考点，绘制该点
    if (useCustomRef)
    {
        painter->setPen(QPen(Qt::green, 1));
        painter->drawEllipse(QPointF(referencePoint), 4, 4);
    }


    if (hasPickedRef) {
        painter->setPen(QPen(Qt::green, 2));
        painter->setBrush(Qt::green);
        painter->drawEllipse(pickedRefPoint, 4, 4);

        // 显示标签（文本）
        QString label;
        if (!pickedRefShape.expired() && pickedRefVertexIndex >= 0) {
            // 如果 shape 有某种可显示 id，使用之；否则显示 "selected shape vertex i"
            label = QString("Ref: vertex %1").arg(pickedRefVertexIndex);
        } else {
            label = QString("Ref: (%1,%2)").arg(int(pickedRefPoint.x())).arg(int(pickedRefPoint.y()));
        }
        painter->setPen(Qt::black);
        painter->drawText(pickedRefPoint + QPointF(6, -6), label);
    }

}

