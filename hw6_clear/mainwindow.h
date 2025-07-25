#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QTabWidget>
#include <QStackedLayout>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QColorDialog> // 添加颜色对话框支持
#include "objmodelcanvas.h"

class BaseCanvasWidget;
class ObjModelCanvas;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    
public slots:
    void updatePointInfo(const QPointF& point);
    void clearPointInfo();
    void showDeleteMessage();
    void updateCanvasView(int index);
    void loadObjModel();
    void resetObjView();
    void toggleShowFaces(bool show);
    void updateAmbientIntensity(int value);
    void updateDiffuseIntensity(int value);
    void updateSpecularIntensity(int value);
    void updateShininess(int value);
    void setDrawMode(int index); // 设置绘制模式
    void changeBackgroundColor(); // 新增：更改背景颜色
    
public:
    void setupObjControls();
    
    QTabWidget *tabWidget;
    ObjModelCanvas *objModelCanvas;
    
    // 右侧控制面板
    QWidget *controlPanel;
    QVBoxLayout *controlLayout;
    QStackedLayout *stackedControlLayout;
    
    // 通用控件
    QLabel *pointInfoLabel;
    QLabel *objInfoLabel;
    QTimer *deleteMessageTimer;
    QMap<QString, QColor> curveColors;
    
    // 光照控制
    QSlider *ambientSlider;
    QSlider *diffuseSlider;
    QSlider *specularSlider;
    QSlider *shininessSlider;
    
    // 绘制模式选择
    QComboBox *drawModeComboBox;
    
    // 背景颜色按钮
    QPushButton *bgColorButton;
};

#endif // MAINWINDOW_H