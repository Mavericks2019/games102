#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QTabWidget>
#include <QStackedLayout>
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