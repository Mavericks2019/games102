#ifndef BEZIERCURVECANVAS_H
#define BEZIERCURVECANVAS_H

#include "basecanvaswidget.h"

class BezierCurveCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit BezierCurveCanvas(QWidget *parent = nullptr);
    
    void toggleCurveVisibility(bool visible) { showCurve = visible; update(); }
    void toggleControlPolygon(bool visible) { showControlPolygon = visible; update(); }
    
    // 重写基类方法
    void drawCurves(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;

private:
    QVector<QPointF> calculateBezierCurve();
    QPointF bezierPoint(double t);
    
    QVector<QPointF> bezierPoints;
    bool showControlPolygon = true;
};

#endif // BEZIERCURVECANVAS_H