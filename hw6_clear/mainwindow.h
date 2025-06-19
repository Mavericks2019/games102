#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QTabWidget>
#include <QStackedLayout>
#include <QCheckBox>
#include "objmodelcanvas.h"

class BaseCanvasWidget;
class ObjModelCanvas;
class QLabel;

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
    void loadObjModel();
    void resetObjView();
    void toggleShowFaces(bool show);  // 新增：切换显示面的槽函数
    
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
    
    // 新增：显示面的复选框
    QCheckBox *showFacesCheckbox;
};

#endif // MAINWINDOW_H