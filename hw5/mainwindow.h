#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QTabWidget>
#include <QStackedLayout>  // 添加头文件

class BaseCanvasWidget;
class ParametricCurveCanvas;
class CubicSplineCanvas;
class BezierCurveCanvas;
class BSplineCanvas;
class QCheckBox;
class QLabel;
class QSlider;
class QRadioButton;
class QButtonGroup;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    
private slots:
    void updatePointInfo(const QPointF& point);
    void clearPointInfo();
    void showDeleteMessage();
    void updateCanvasView(int index);
    
    // 参数曲线方法
    void updateDegreeValue(int value);
    void updateSigmaValue(int value);
    void updateLambdaValue(int value);
    void parameterizationMethodChanged(int id);
    
    // 通用控制
    void toggleCurveVisibility(bool visible);
    void toggleControlPointsVisibility(bool visible);
    void toggleControlPolygonVisibility(bool visible);
    void setBSplineDegree(int degree);

private:
    QHBoxLayout* createSliderLayout(QSlider* slider, QLabel* valueLabel);
    void setupParametricControls();
    void setupSplineControls();
    void setupBezierControls();
    void setupBSplineControls();
    
    QTabWidget *tabWidget;
    ParametricCurveCanvas *parametricCanvas;
    CubicSplineCanvas *splineCanvas;
    BezierCurveCanvas *bezierCanvas;
    BSplineCanvas *bSplineCanvas;
    
    // 右侧控制面板
    QWidget *controlPanel;
    QVBoxLayout *controlLayout;
    QStackedLayout *stackedControlLayout;  // 添加堆叠布局
    
    // 参数曲线控件
    QCheckBox *polyInterpCheck;
    QCheckBox *gaussInterpCheck;
    QCheckBox *leastSquaresCheck;
    QCheckBox *ridgeRegCheck;
    QSlider *sigmaSlider;
    QSlider *lambdaSlider;
    QLabel *sigmaValueLabel;
    QLabel *lambdaValueLabel;
    
    // 通用控件
    QCheckBox *showCurveCheck;
    QCheckBox *showControlPointsCheck;
    QCheckBox *showControlPolygonCheck;
    QSlider *degreeSlider;
    QLabel *degreeValueLabel;
    QLabel *pointInfoLabel;
    
    QTimer *deleteMessageTimer;
    QMap<QString, QColor> curveColors;
};

#endif // MAINWINDOW_H