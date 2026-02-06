// mainwindow.h 开头添加
#include "tangramgame.h"
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include "selecttool.h"
#include "polygontool.h"
#include "filltool.h"

// 提前声明 TangramFigure 枚举（关键修改：避免编译错误）
// 修改后的正确代码
#include "tangramgame.h" // 引入枚举类的唯一定义
enum class TangramFigure; // 仅保留前向声明，不重复定义枚举值

class CanvasWidget;
class BaseTool;
class DrawEngine;
class QAction;
class TangramGame;
class TangramTool;

/**
 * @brief MainWindow —— 应用程序主窗口
 * 负责整体 UI 布局、工具栏初始化、工具切换逻辑
 *
 * 内部结构：
 * - CanvasWidget：显示绘图画布
 * - DrawEngine：绘图引擎（负责像素操作）
 * - BaseTool：当前激活的鼠标工具（如画线、选择等）
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectLineTool();                                                      // 切换到画线工具
    void selectArcTool();                                                       // 切换到圆弧工具
    void selectSelectTool();                                                    // 切换到选择工具

private:
    void initTools();                                                           // 初始化工具实例
    void initUI();                                                              // 初始化UI界面（工具栏、画布等）

private:
    CanvasWidget* canvas;                                                       // 画布部件（显示绘图）
    DrawEngine* drawEngine;                                                     // 绘图引擎（执行算法绘制）
    BaseTool* currentTool;                                                      // 当前使用的鼠标工具
    BaseTool* lineTool;                                                         // 画线工具
    BaseTool* arcTool;                                                          // 圆弧工具
    PolygonTool* polygonTool;
    BaseTool* clipTool;
    SelectTool* selectTool;                                                     // 选择工具
    FillTool* fillTool;
    QCheckBox* fillPolygonsCheckbox;
    TangramGame* tangramGame;
    TangramTool* tangramTool;
    QPushButton* tangramPlayButton;
    QComboBox* tangramFigureCombo;
    QAction* tangramToolAction;
    QDoubleSpinBox* tangramRotateSpin;
    QPushButton* tangramRotateApply;
    QDockWidget* transformDock;
    QLineEdit *txEdit, *tyEdit, *sxEdit, *syEdit, *angleEdit;
    QPushButton *applyTransformBtn;
    QRadioButton *refCentroidBtn, *refCustomBtn;
    // UI控件
    QSlider* penWidthSlider;                                                    // 线宽控件
    QComboBox* lineStyleComboBox;                                               // 线型控件
    QComboBox* lineCapComboBox;                                                 // 线帽控件
};

#endif // MAINWINDOW_H
