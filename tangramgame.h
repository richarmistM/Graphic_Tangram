#ifndef TANGRAMGAME_H
#define TANGRAMGAME_H
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <array>
#include <memory>
#include "tangrampiece.h"

class DrawEngine;

// 新增房子和正方形枚举值
enum class TangramFigure {
    Free = 0,
    Heart,
    House,       // 新增：房子造型
    Square       // 新增：正方形造型
};

class TangramGame : public QObject
{
    Q_OBJECT
public:
    explicit TangramGame(DrawEngine* engine, QObject* parent = nullptr);
    void initialize();
    const std::array<std::shared_ptr<TangramPiece>, 7>& pieces() const { return piecesStorage; }
    void scatter();
    std::shared_ptr<TangramPiece> pieceAt(const QPoint& canvasPos) const;
    void bringToFront(const std::shared_ptr<TangramPiece>& piece);
    bool snapPieceToTarget(const std::shared_ptr<TangramPiece>& piece,
                           double posThreshold,
                           double angleThresholdDeg);
    void setInteractiveTarget(TangramFigure fig);
    TangramFigure interactiveTarget() const { return currentTarget; }
    // 新增：指定目标造型启动演示
    void startDemo(TangramFigure targetFig = TangramFigure::Heart);
    void stopDemo();
    bool isAnimating() const { return demoPhase != DemoPhase::Idle; }

signals:
    void requestCanvasUpdate();

private slots:
    void onAnimationTick();

private:
    enum class DemoPhase {
        Idle,
        ToTarget,    // 通用目标阶段（替代原ToHeart）
        Hold
    };

    void ensurePiecesLoaded();
    void setAllPiecesTo(const std::array<TangramPose, 7>& poses);
    std::array<TangramPose, 7> currentPoses() const;
    const std::array<TangramPose, 7>& posesForFigure(TangramFigure fig) const;
    int indexOfPiece(const std::shared_ptr<TangramPiece>& piece) const;
    double shortestAngleDelta(double fromDeg, double toDeg) const;

private:
    DrawEngine* drawEngine;
    bool initialized;
    std::array<std::shared_ptr<TangramPiece>, 7> piecesStorage;
    std::array<TangramPose, 7> scatterPoses;
    std::array<TangramPose, 7> heartPoses;
    std::array<TangramPose, 7> housePoses;    // 新增：房子造型姿态
    std::array<TangramPose, 7> squarePoses;   // 新增：正方形造型姿态
    TangramFigure currentTarget;
    TangramFigure demoTargetFig;              // 新增：当前演示目标

    // animation state
    QTimer animationTimer;
    QElapsedTimer animationClock;
    qint64 lastFrameMs;
    DemoPhase demoPhase;
    double phaseElapsed;
    double phaseDuration;
    std::array<TangramPose, 7> phaseStartPoses;
    std::array<TangramPose, 7> phaseTargetPoses;
};

#endif // TANGRAMGAME_H
