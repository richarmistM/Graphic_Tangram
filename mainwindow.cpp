#include "mainwindow.h"
#include "drawengine.h"
#include "canvaswidget.h"
#include "linetool.h"
#include "arctool.h"
#include "polygontool.h"
#include "cliptool.h"
#include "selecttool.h"
#include "filltool.h"
#include "tangramgame.h"
#include "tangramtool.h"
#include <QToolBar>
#include <QAction>
#include <QDockWidget>
#include <QLineEdit>
#include <QFormLayout>
#include <QCheckBox>
#include <QDebug>
#include <QString>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    canvas(nullptr),
    drawEngine(nullptr),
    currentTool(nullptr),
    lineTool(nullptr),
    arcTool(nullptr),
    selectTool(nullptr),
    tangramGame(nullptr),
    tangramTool(nullptr),
    tangramPlayButton(nullptr),
    tangramFigureCombo(nullptr),
    tangramToolAction(nullptr),
    tangramRotateSpin(nullptr),
    tangramRotateApply(nullptr)
{
    initTools();                                                                // 初始化工具实例
    initUI();                                                                   // 初始化UI界面
}

MainWindow::~MainWindow()
{
    // canvas 不会在 Qt 的容器中自动释放
    delete lineTool;
    delete arcTool;
    delete polygonTool;
    delete clipTool;
    delete selectTool;
    delete tangramTool;
    delete drawEngine;                                                          // DrawEngine 由 MainWindow 统一管理
}

/**
 * @brief 初始化工具
 *
 * 每种工具继承自 BaseTool
 */
void MainWindow::initTools()
{
    // 创建工具实例
    lineTool = new LineTool();
    arcTool = new ArcTool();
    polygonTool = new PolygonTool();
    clipTool = new ClipTool();
    selectTool = new SelectTool();
    fillTool = new FillTool();
    currentTool = lineTool;                                                     // 默认工具
}

/**
 * @brief 初始化主窗口布局
 * 包括：
 * - 创建 DrawEngine 引擎
 * - 创建 CanvasWidget 并绑定 DrawEngine
 * - 构建工具栏并连接按钮事件
 */
void MainWindow::initUI()
{
    setWindowTitle("2D绘图演示");
    resize(800, 600);
    drawEngine = new DrawEngine(800, 600);                                      // 初始化绘图引擎
    // 创建画布并绑定当前工具
    canvas = new CanvasWidget(drawEngine, this);
    tangramGame = new TangramGame(drawEngine, this);
    tangramGame->initialize();
    tangramTool = new TangramTool(tangramGame, canvas);
    connect(tangramGame, &TangramGame::requestCanvasUpdate, this, [=](){
        if (canvas) canvas->update();
    });
    canvas->setTool(currentTool);
    setCentralWidget(canvas);

    // ------------------- 顶部工具栏 -------------------
    // 创建工具栏
    QToolBar* toolbar = addToolBar("Tools");
    // 添加工具按钮
    QAction* lineAction = toolbar->addAction("Line");
    QAction* arcAction = toolbar->addAction("Arc");
    QAction* selectAction = toolbar->addAction("Select");
    // 连接信号：点击按钮 → 切换工具
    connect(lineAction, &QAction::triggered, this, &MainWindow::selectLineTool);
    connect(arcAction, &QAction::triggered, this, &MainWindow::selectArcTool);
    connect(selectAction, &QAction::triggered, this, &MainWindow::selectSelectTool);

    QAction* polyAction = toolbar->addAction("Polygon");
    connect(polyAction, &QAction::triggered, this, [=](){
        currentTool = polygonTool;
        canvas->setTool(currentTool);
        if (tangramToolAction) tangramToolAction->setChecked(false);
    });

    QAction* clipAction = toolbar->addAction("Clip");
    connect(clipAction, &QAction::triggered, this, [=](){
        currentTool = clipTool;
        canvas->setTool(currentTool);
        if (tangramToolAction) tangramToolAction->setChecked(false);
    });

    // ---------- 填充工具按钮 ----------
    QAction* fillAction = toolbar->addAction("Fill");
    connect(fillAction, &QAction::triggered, this, [=](){
        currentTool = fillTool;
        canvas->setTool(currentTool);
        if (tangramToolAction) tangramToolAction->setChecked(false);
    });

    toolbar->addSeparator();

    // 七巧板工具相关
    tangramToolAction = toolbar->addAction("Tangram");
    tangramToolAction->setCheckable(true);
    connect(tangramToolAction, &QAction::triggered, this, [=]()
            {
                if (!tangramTool) return;
                currentTool = tangramTool;
                canvas->setTool(currentTool);
                tangramToolAction->setChecked(true);
            });

    QPushButton* tangramResetButton = new QPushButton("Scatter", this);
    toolbar->addWidget(tangramResetButton);
    connect(tangramResetButton, &QPushButton::clicked, this, [=]() {
        if (tangramGame) tangramGame->scatter();
        if (tangramTool) tangramTool->clearSelection();
        if (tangramFigureCombo) tangramFigureCombo->setCurrentIndex(0);
    });

    // 新增：三个独立的演示按钮（心形、房子、正方形）
    QPushButton* demoHeartBtn = new QPushButton("Demo: Heart", this);
    toolbar->addWidget(demoHeartBtn);
    connect(demoHeartBtn, &QPushButton::clicked, this, [=](){
        if (tangramGame) tangramGame->startDemo(TangramFigure::Heart);
    });

    QPushButton* demoHouseBtn = new QPushButton("Demo: House", this);
    toolbar->addWidget(demoHouseBtn);
    connect(demoHouseBtn, &QPushButton::clicked, this, [=](){
        if (tangramGame) tangramGame->startDemo(TangramFigure::House);
    });

    QPushButton* demoSquareBtn = new QPushButton("Demo: Square", this);
    toolbar->addWidget(demoSquareBtn);
    connect(demoSquareBtn, &QPushButton::clicked, this, [=](){
        if (tangramGame) tangramGame->startDemo(TangramFigure::Square);
    });

    // 七巧板旋转控制
    tangramRotateSpin = new QDoubleSpinBox(this);
    tangramRotateSpin->setRange(-360.0, 360.0);
    tangramRotateSpin->setSingleStep(5.0);
    tangramRotateSpin->setDecimals(1);
    tangramRotateSpin->setSuffix(QStringLiteral(" deg"));
    toolbar->addWidget(tangramRotateSpin);

    tangramRotateApply = new QPushButton("Rotate", this);
    toolbar->addWidget(tangramRotateApply);
    connect(tangramRotateApply, &QPushButton::clicked, this, [=]() {
        if (!tangramTool) return;
        double angle = tangramRotateSpin ? tangramRotateSpin->value() : 0.0;
        if (std::abs(angle) < 1e-6) return;
        if (tangramTool->rotateSelectionBy(angle)) {
            tangramRotateSpin->setValue(0.0);
        }
    });

    // 七巧板目标选择下拉框（新增房子、正方形选项）
    tangramFigureCombo = new QComboBox(this);
    tangramFigureCombo->addItem("Free Play");
    tangramFigureCombo->addItem("Target: Heart");
    tangramFigureCombo->addItem("Target: House");
    tangramFigureCombo->addItem("Target: Square");
    toolbar->addWidget(tangramFigureCombo);
    connect(tangramFigureCombo, &QComboBox::currentIndexChanged, this, [=](int index){
        if (!tangramGame) return;
        switch(index) {
        case 1: tangramGame->setInteractiveTarget(TangramFigure::Heart); break;
        case 2: tangramGame->setInteractiveTarget(TangramFigure::House); break;
        case 3: tangramGame->setInteractiveTarget(TangramFigure::Square); break;
        default: tangramGame->setInteractiveTarget(TangramFigure::Free); break;
        }
    });

    // 默认设置为自由模式
    if (tangramGame)
        tangramGame->setInteractiveTarget(TangramFigure::Free);

    // 初始激活七巧板工具
    if (tangramTool)
    {
        currentTool = tangramTool;
        canvas->setTool(currentTool);
        if (tangramToolAction)
            tangramToolAction->setChecked(true);
    }

    // ---------- 多边形绘制时是否填充 checkbox ----------
    fillPolygonsCheckbox = new QCheckBox("Fill polygons", this);
    fillPolygonsCheckbox->setChecked(false);
    toolbar->addWidget(fillPolygonsCheckbox);
    // 连接 Checkbox 到 polygonTool
    connect(fillPolygonsCheckbox, &QCheckBox::toggled, this, [=](bool checked){
        if (polygonTool) polygonTool->setFillOnComplete(checked);
    });

    // 清除画布按钮
    QAction* clearAction = toolbar->addAction("clear");
    connect(clearAction, &QAction::triggered, this, [=](){
        if (!drawEngine) return;
        drawEngine->clearAllShapes();
        if (tangramGame) {
            for (const auto& piece : tangramGame->pieces())
                drawEngine->addShape(piece);
            tangramGame->scatter();
            if (tangramFigureCombo) {
                int idx = tangramFigureCombo->currentIndex();
                switch (idx) {
                case 1: tangramGame->setInteractiveTarget(TangramFigure::Heart); break;
                case 2: tangramGame->setInteractiveTarget(TangramFigure::House); break;
                case 3: tangramGame->setInteractiveTarget(TangramFigure::Square); break;
                default: tangramGame->setInteractiveTarget(TangramFigure::Free); break;
                }
            }
        }
        if (canvas) canvas->update();
    });

    // ------------------- 线型 ComboBox -------------------
    QComboBox* lineTypeBox = new QComboBox(this);
    lineTypeBox->addItem("Solid");
    lineTypeBox->addItem("Dash");
    lineTypeBox->addItem("Dot");
    lineTypeBox->addItem("DashDot");
    toolbar->addWidget(lineTypeBox);
    connect(lineTypeBox, &QComboBox::currentTextChanged, this, [=](const QString &text){
        if(drawEngine) drawEngine->setLineStyle(text);
    });

    // ------------------- 线帽 ComboBox -------------------
    QComboBox* lineCapBox = new QComboBox(this);
    lineCapBox->addItem("Flat");
    lineCapBox->addItem("Square");
    lineCapBox->addItem("Round");
    toolbar->addWidget(lineCapBox);
    connect(lineCapBox, &QComboBox::currentTextChanged, this, [=](const QString &text){
        if(drawEngine) drawEngine->setLineCap(text);
    });

    // ------------------- 左侧滑块：线宽 -------------------
    penWidthSlider = new QSlider(Qt::Vertical, this);
    penWidthSlider->setRange(1,20);
    penWidthSlider->setValue(drawEngine->getPenWidth());
    penWidthSlider->setTickPosition(QSlider::TicksRight);
    penWidthSlider->setTickInterval(1);
    QDockWidget* dock = new QDockWidget("Line Width", this);
    dock->setWidget(penWidthSlider);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    connect(penWidthSlider, &QSlider::valueChanged, this, [=](int value){
        if(drawEngine)
            drawEngine->setPenWidth(value);                                     // 滑块改变线宽
    });

    // 变换面板
    transformDock = new QDockWidget("Transform", this);
    QWidget* dockW = new QWidget(transformDock);
    QFormLayout* fl = new QFormLayout(dockW);
    txEdit = new QLineEdit("0"); tyEdit = new QLineEdit("0");
    sxEdit = new QLineEdit("1"); syEdit = new QLineEdit("1");
    angleEdit = new QLineEdit("0");
    refCentroidBtn = new QRadioButton("Centroid"); refCustomBtn = new QRadioButton("Custom point");
    refCentroidBtn->setChecked(true);
    applyTransformBtn = new QPushButton("Apply");
    fl->addRow("Translate X:", txEdit);
    fl->addRow("Translate Y:", tyEdit);
    fl->addRow("Scale X:", sxEdit);
    fl->addRow("Scale Y:", syEdit);
    fl->addRow("Rotate (deg):", angleEdit);
    fl->addRow(refCentroidBtn);
    fl->addRow(refCustomBtn);
    fl->addRow(applyTransformBtn);
    dockW->setLayout(fl);
    transformDock->setWidget(dockW);
    addDockWidget(Qt::RightDockWidgetArea, transformDock);

    // 连接 apply 按钮
    connect(applyTransformBtn, &QPushButton::clicked, this, [=](){
        if (!selectTool || !drawEngine) return;
        double tx = txEdit->text().toDouble();
        double ty = tyEdit->text().toDouble();
        double sx = sxEdit->text().toDouble();
        double sy = syEdit->text().toDouble();
        double angle = angleEdit->text().toDouble();
        // 参考点
        QPointF ref(0,0);
        bool useCustom = refCustomBtn->isChecked();
        selectTool->pickRefMode = useCustom;
        if (useCustom && selectTool && selectTool->isRefPicked()) {
            ref = selectTool->getPickedRefPoint();
        } else {
            const auto &ss = selectTool->getSelection();
            if (ss.empty()) return;
            QPointF sum(0,0);
            for (auto &s : ss) sum += s->centroid();
            ref = QPointF(sum.x() / ss.size(), sum.y() / ss.size());
        }
        selectTool->applyTransformToSelection_params(tx, ty, sx, sy, angle, ref, drawEngine);
    });
}

/**
 * @brief 切换到画线工具
 */
void MainWindow::selectLineTool()
{
    currentTool = lineTool;
    canvas->setTool(currentTool);
    if (tangramToolAction) tangramToolAction->setChecked(false);
}

/**
 * @brief 切换到圆弧工具
 */
void MainWindow::selectArcTool()
{
    currentTool = arcTool;
    canvas->setTool(currentTool);
    if (tangramToolAction) tangramToolAction->setChecked(false);
}

/**
 * @brief 切换到选择工具
 */
void MainWindow::selectSelectTool()
{
    currentTool = selectTool;
    canvas->setTool(currentTool);
    if (tangramToolAction) tangramToolAction->setChecked(false);
}
