#ifndef CUBICSPLINECANVAS_H
#define CUBICSPLINECANVAS_H

#include "basecanvaswidget.h"
#include <vector>
#include <Eigen/Dense>

class CubicSplineCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit CubicSplineCanvas(QWidget *parent = nullptr);
    
    void toggleCurveVisibility(bool visible) { showCurve = visible; update(); }
    void toggleControlPointsVisibility(bool visible) { showControlPoints = visible; update(); }
    
    // 重写基类方法
    void drawCurves(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void clearPoints() override; // 添加重写的clearPoints声明
    
private:
    struct ControlPoint {
        QPointF pos;
        bool moving = false;
        bool selected = false;
        QPointF leftTangent = QPointF(-20, 0);
        QPointF rightTangent = QPointF(20, 0);
        bool leftTangentFixed = false;
        bool rightTangentFixed = false;
    };
    
    void calculateSpline();
    void calculateSplineNaive();
    double distance(const QPointF &p1, const QPointF &p2);
    void updateAdjacentTangents(int index);
    
    QVector<ControlPoint> points;
    QVector<QPointF> splinePoints;
    bool showControlPoints = true;
    int selectedIndex = -1;
    int hoveredIndex = -1;
    bool draggingLeftTangent = false;
    bool draggingRightTangent = false;
};

#endif // CUBICSPLINECANVAS_H