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
    void deletePoint(int index) override;
    void drawInfoPanel(QPainter &painter) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    struct ControlPoint {
        QPointF pos;
        bool isTangent = false;
        int parentIndex = -1;
        bool leftTangent = true;
    };
     void updateControlPoints();
    void calculateSpline();
    void showDerivatives(int pointIndex);
    
    QVector<ControlPoint> controlPoints;
    QVector<QPointF> splinePoints;
    int selectedControlPoint = -1;
    bool showControlPoints = true;
    int currentPointIndex = -1;
};

#endif // CUBICSPLINECANVAS_H