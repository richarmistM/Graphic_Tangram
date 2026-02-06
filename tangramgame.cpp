#include "tangramgame.h"
#include "drawengine.h"
#include <QtMath>
#include <QPoint>
#include <QLineF>
#include <cmath>

namespace {
constexpr double SMALL_SIZE = 100.0;
constexpr double LARGE_SIZE = 200.0;
const double MEDIUM_SIZE = SMALL_SIZE * std::sqrt(2.0);
constexpr double PARA_OFFSET = SMALL_SIZE;

QPointF polygonCentroid(const std::vector<QPointF>& pts)
{
    if (pts.empty()) return QPointF(0.0, 0.0);
    double area = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    int n = static_cast<int>(pts.size());
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
        for (const auto& p : pts) { sx += p.x(); sy += p.y(); }
        return QPointF(sx / pts.size(), sy / pts.size());
    }
    cx /= (6.0 * area);
    cy /= (6.0 * area);
    return QPointF(cx, cy);
}

QPointF rotatePoint(const QPointF& p, double angleDeg)
{
    double rad = qDegreesToRadians(angleDeg);
    double c = std::cos(rad);
    double s = std::sin(rad);
    return QPointF(p.x() * c - p.y() * s,
                   p.x() * s + p.y() * c);
}

std::vector<QPointF> makeTriangle(double size)
{
    return {
        QPointF(0.0, 0.0),
        QPointF(size, 0.0),
        QPointF(0.0, size)
    };
}

std::vector<QPointF> makeSquare(double size)
{
    return {
        QPointF(0.0, 0.0),
        QPointF(size, 0.0),
        QPointF(size, size),
        QPointF(0.0, size)
    };
}

std::vector<QPointF> makeParallelogram(double size, double offset)
{
    return {
        QPointF(0.0, 0.0),
        QPointF(size, 0.0),
        QPointF(size + offset, size),
        QPointF(offset, size)
    };
}

double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

std::vector<QPointF> basePolygonFor(TangramPieceType type)
{
    switch (type) {
    case TangramPieceType::LargeA:
    case TangramPieceType::LargeB:
        return makeTriangle(LARGE_SIZE);
    case TangramPieceType::Medium:
        return makeTriangle(MEDIUM_SIZE);
    case TangramPieceType::SmallA:
    case TangramPieceType::SmallB:
        return makeTriangle(SMALL_SIZE);
    case TangramPieceType::Square:
        return makeSquare(SMALL_SIZE);
    case TangramPieceType::Parallelogram:
        return makeParallelogram(SMALL_SIZE, PARA_OFFSET);
    }
    return {};
}

TangramPose poseFromAnchor(TangramPieceType type,
                           const QPointF& anchor,
                           double rotationDeg,
                           bool flipped)
{
    auto base = basePolygonFor(type);
    QPointF centroid = polygonCentroid(base);
    QPointF localAnchor = base.front() - centroid;
    if (flipped)
        localAnchor.setX(-localAnchor.x());
    QPointF rotatedAnchor = rotatePoint(localAnchor, rotationDeg);
    QPointF translation = anchor - rotatedAnchor;
    return TangramPose{translation, rotationDeg, flipped};
}
} // namespace

TangramGame::TangramGame(DrawEngine* engine, QObject* parent)
    : QObject(parent),
    drawEngine(engine),
    initialized(false),
    currentTarget(TangramFigure::Free),
    demoTargetFig(TangramFigure::Heart),
    lastFrameMs(0),
    demoPhase(DemoPhase::Idle),
    phaseElapsed(0.0),
    phaseDuration(0.0)
{
    animationTimer.setInterval(16);
    connect(&animationTimer, &QTimer::timeout, this, &TangramGame::onAnimationTick);
}

void TangramGame::initialize()
{
    ensurePiecesLoaded();
    scatter();
}

void TangramGame::ensurePiecesLoaded()
{
    if (initialized || !drawEngine) return;

    auto largeVerts = makeTriangle(LARGE_SIZE);
    auto mediumVerts = makeTriangle(MEDIUM_SIZE);
    auto smallVerts = makeTriangle(SMALL_SIZE);
    auto squareVerts = makeSquare(SMALL_SIZE);
    auto paraVerts = makeParallelogram(SMALL_SIZE, PARA_OFFSET);

    piecesStorage[0] = std::make_shared<TangramPiece>(TangramPieceType::LargeA, largeVerts);
    piecesStorage[1] = std::make_shared<TangramPiece>(TangramPieceType::LargeB, largeVerts);
    piecesStorage[2] = std::make_shared<TangramPiece>(TangramPieceType::Medium, mediumVerts);
    piecesStorage[3] = std::make_shared<TangramPiece>(TangramPieceType::Square, squareVerts);
    piecesStorage[4] = std::make_shared<TangramPiece>(TangramPieceType::SmallA, smallVerts);
    piecesStorage[5] = std::make_shared<TangramPiece>(TangramPieceType::SmallB, smallVerts);
    piecesStorage[6] = std::make_shared<TangramPiece>(TangramPieceType::Parallelogram, paraVerts);

    for (const auto& piece : piecesStorage)
    {
        if (piece) {
            piece->color = Qt::black;
            piece->penWidth = 2;
            drawEngine->addShape(piece);
        }
    }

    // 原分散姿态定义
    scatterPoses = {
        TangramPose{QPointF(180.0, 140.0),  -10.0, false},
        TangramPose{QPointF(620.0, 180.0),   95.0, false},
        TangramPose{QPointF(520.0, 360.0),  -35.0, false},
        TangramPose{QPointF(300.0, 300.0),   20.0, false},
        TangramPose{QPointF(160.0, 420.0),   40.0, false},
        TangramPose{QPointF(650.0, 320.0),  -70.0, false},
        TangramPose{QPointF(420.0, 200.0),   15.0, false}
    };

    // 原心形姿态定义
    const double unit = SMALL_SIZE / 2.0;
    const double root2 = std::sqrt(2.0);
    const double offsetX = 240.0;
    const double offsetY = 260.0;
    auto heartAnchor = [&](double mulSqrt2X, double mulSqrt2Y) {
        return QPointF(offsetX + mulSqrt2X * root2 * unit,
                       offsetY + mulSqrt2Y * root2 * unit);
    };
    heartPoses = {
        poseFromAnchor(TangramPieceType::LargeA,        heartAnchor(2.0,  2.0), 135.0, true),
        poseFromAnchor(TangramPieceType::LargeB,        heartAnchor(2.0, -2.0),  45.0, false),
        poseFromAnchor(TangramPieceType::Medium,        heartAnchor(4.0, -2.0),   0.0, true),
        poseFromAnchor(TangramPieceType::Square,        heartAnchor(5.0,  1.0),  45.0, true),
        poseFromAnchor(TangramPieceType::SmallA,        heartAnchor(6.0,  0.0),  45.0, true),
        poseFromAnchor(TangramPieceType::SmallB,        heartAnchor(3.0, -3.0), -45.0, true),
        poseFromAnchor(TangramPieceType::Parallelogram, heartAnchor(5.0,  1.0), 225.0, false)
    };






    // 新增：房子造型姿态（参考心形坐标逻辑，统一单位和锚点计算）
    const double houseOffsetX = 240.0;
    const double houseOffsetY = 260.0;
    auto houseAnchor = [&](double mulSqrt2X, double mulSqrt2Y) {
        return QPointF(houseOffsetX + mulSqrt2X * root2 * unit,
                       houseOffsetY + mulSqrt2Y * root2 * unit);
    };
    housePoses = {
        poseFromAnchor(TangramPieceType::LargeA,        houseAnchor(0.5,  -1.0),  45.0, false),  // 屋顶左半
        poseFromAnchor(TangramPieceType::LargeB,        houseAnchor(1.5,  3.0),  135.0, true),   // 屋顶右半
        poseFromAnchor(TangramPieceType::Medium,        houseAnchor(3.5,  3.0), 180.0, false),  // 房身主体
        poseFromAnchor(TangramPieceType::Square,        houseAnchor(1.0,  -2.0),   0.0, false),  // 房门
        poseFromAnchor(TangramPieceType::SmallA,        houseAnchor(0.5,  2),   45.0, false),  // 左窗
        poseFromAnchor(TangramPieceType::SmallB,        houseAnchor(0.5,  2),   135.0, false),  // 右窗
        poseFromAnchor(TangramPieceType::Parallelogram, houseAnchor(1.0,  -0.5),  0.0, false)   // 房底装饰
    };

    // 新增：正方形造型姿态（参考心形坐标逻辑，统一单位和锚点计算）
    const double squareOffsetX = 240.0;
    const double squareOffsetY = 260.0;
    auto squareAnchor = [&](double mulSqrt2X, double mulSqrt2Y) {
        return QPointF(squareOffsetX + mulSqrt2X * root2 * unit,
                       squareOffsetY + mulSqrt2Y * root2 * unit);
    };
    squarePoses = {
        poseFromAnchor(TangramPieceType::LargeA,        squareAnchor(0.0,  0.0),  45.0, false),  // 左上大三角
        poseFromAnchor(TangramPieceType::LargeB,        squareAnchor(2.0,  0.0), 135.0, false),  // 右上大三角
        poseFromAnchor(TangramPieceType::Medium,        squareAnchor(1.0,  1.0),  45.0, true),   // 中间中三角
        poseFromAnchor(TangramPieceType::Square,        squareAnchor(1.0,  0.5),   0.0, false),  // 上正方形
        poseFromAnchor(TangramPieceType::SmallA,        squareAnchor(0.5,  1.5),  45.0, false),  // 左下小三角
        poseFromAnchor(TangramPieceType::SmallB,        squareAnchor(1.5,  1.5), 135.0, false),  // 右下小三角
        poseFromAnchor(TangramPieceType::Parallelogram, squareAnchor(1.0,  1.5),  45.0, true)   // 下平行四边形
    };

    initialized = true;
}






void TangramGame::scatter()
{
    ensurePiecesLoaded();
    setAllPiecesTo(scatterPoses);
    currentTarget = TangramFigure::Free;
    emit requestCanvasUpdate();
}

std::shared_ptr<TangramPiece> TangramGame::pieceAt(const QPoint& canvasPos) const
{
    for (int i = static_cast<int>(piecesStorage.size()) - 1; i >= 0; --i)
    {
        const auto& piece = piecesStorage[static_cast<std::size_t>(i)];
        if (piece && piece->contains(canvasPos))
            return piece;
    }
    return nullptr;
}

void TangramGame::bringToFront(const std::shared_ptr<TangramPiece>& piece)
{
    if (!drawEngine || !piece) return;
    if (drawEngine->removeShape(piece))
        drawEngine->addShape(piece);
}

bool TangramGame::snapPieceToTarget(const std::shared_ptr<TangramPiece>& piece,
                                    double posThreshold,
                                    double angleThresholdDeg)
{
    if (!piece) return false;
    if (currentTarget == TangramFigure::Free) return false;
    int idx = indexOfPiece(piece);
    if (idx < 0) return false;
    const auto& targetSet = posesForFigure(currentTarget);
    const TangramPose& target = targetSet[static_cast<std::size_t>(idx)];
    const TangramPose& now = piece->pose();
    double dist = QLineF(now.position, target.position).length();
    double angleDelta = std::abs(shortestAngleDelta(now.rotationDeg, target.rotationDeg));
    bool flipOk = (now.flipped == target.flipped);
    if (dist <= posThreshold && angleDelta <= angleThresholdDeg && flipOk)
    {
        piece->setPose(target);
        emit requestCanvasUpdate();
        return true;
    }
    return false;
}

void TangramGame::setInteractiveTarget(TangramFigure fig)
{
    currentTarget = fig;
}

// 新增：支持指定目标造型的演示启动函数
void TangramGame::startDemo(TangramFigure targetFig)
{
    if (isAnimating()) return;
    ensurePiecesLoaded();
    currentTarget = TangramFigure::Free;
    demoTargetFig = targetFig;
    phaseStartPoses = currentPoses();
    phaseTargetPoses = posesForFigure(demoTargetFig);
    demoPhase = DemoPhase::ToTarget;
    phaseElapsed = 0.0;
    phaseDuration = 2.5; // 动画时长2.5秒，与心形一致
    animationClock.start();
    lastFrameMs = animationClock.elapsed();
    animationTimer.start();
}

void TangramGame::stopDemo()
{
    animationTimer.stop();
    demoPhase = DemoPhase::Idle;
    phaseElapsed = 0.0;
    phaseDuration = 0.0;
    emit requestCanvasUpdate();
}

void TangramGame::onAnimationTick()
{
    if (demoPhase == DemoPhase::Idle)
    {
        animationTimer.stop();
        return;
    }

    qint64 nowMs = animationClock.elapsed();
    double dt = (nowMs - lastFrameMs) / 1000.0;
    lastFrameMs = nowMs;
    if (dt <= 0.0) dt = 0.016;

    phaseElapsed += dt;
    double t = clamp01(phaseElapsed / phaseDuration);

    // 通用目标动画（适配心形、房子、正方形）
    if (demoPhase == DemoPhase::ToTarget)
    {
        for (std::size_t i = 0; i < piecesStorage.size(); ++i)
        {
            auto& piece = piecesStorage[i];
            const TangramPose& startPose = phaseStartPoses[i];
            const TangramPose& targetPose = phaseTargetPoses[i];
            QPointF pos = startPose.position + (targetPose.position - startPose.position) * t;
            double angle = startPose.rotationDeg + shortestAngleDelta(startPose.rotationDeg, targetPose.rotationDeg) * t;
            bool flip = startPose.flipped;
            if (startPose.flipped != targetPose.flipped)
                flip = (t >= 0.5) ? targetPose.flipped : startPose.flipped;
            piece->setPose({pos, angle, flip});
        }
    }

    emit requestCanvasUpdate();

    if (t >= 0.999)
    {
        setAllPiecesTo(phaseTargetPoses);
        if (demoPhase == DemoPhase::ToTarget)
        {
            demoPhase = DemoPhase::Hold;
            phaseDuration = 0.5; // 停留0.5秒
            phaseElapsed = 0.0;
            phaseStartPoses = phaseTargetPoses;
            phaseTargetPoses = phaseTargetPoses;
        }
        else if (demoPhase == DemoPhase::Hold)
        {
            stopDemo();
            setInteractiveTarget(demoTargetFig); // 演示结束后切换到对应目标交互模式
        }
    }
}

void TangramGame::setAllPiecesTo(const std::array<TangramPose, 7>& poses)
{
    for (std::size_t i = 0; i < piecesStorage.size(); ++i)
    {
        if (piecesStorage[i])
            piecesStorage[i]->setPose(poses[i]);
    }
}

std::array<TangramPose, 7> TangramGame::currentPoses() const
{
    std::array<TangramPose, 7> result{};
    for (std::size_t i = 0; i < piecesStorage.size(); ++i)
    {
        if (piecesStorage[i])
            result[i] = piecesStorage[i]->pose();
    }
    return result;
}

// 新增：返回对应造型的姿态集合
const std::array<TangramPose, 7>& TangramGame::posesForFigure(TangramFigure fig) const
{
    switch (fig) {
    case TangramFigure::Heart: return heartPoses;
    case TangramFigure::House: return housePoses;   // 房子姿态
    case TangramFigure::Square: return squarePoses; // 正方形姿态
    default: return scatterPoses;
    }
}

int TangramGame::indexOfPiece(const std::shared_ptr<TangramPiece>& piece) const
{
    for (std::size_t i = 0; i < piecesStorage.size(); ++i)
    {
        if (piecesStorage[i] == piece)
            return static_cast<int>(i);
    }
    return -1;
}

double TangramGame::shortestAngleDelta(double fromDeg, double toDeg) const
{
    double diff = std::fmod(toDeg - fromDeg, 360.0);
    if (diff < -180.0) diff += 360.0;
    if (diff > 180.0) diff -= 360.0;
    return diff;
}
