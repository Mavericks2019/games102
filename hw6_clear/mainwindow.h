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
#include "objmodelcanvas.h"

class BaseCanvasWidget;
class ObjModelCanvas;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
        // 光照控制
    QSlider *ambientSlider;
    QSlider *diffuseSlider;
    QSlider *specularSlider;
    QSlider *shininessSlider;  // 新增：高光指数滑块
    
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
    void updateShininess(int value);  // 新增：高光指数控制
    
private:
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
    
};

#endif // MAINWINDOW_H