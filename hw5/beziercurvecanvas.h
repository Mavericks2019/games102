#ifndef BEZIERCURVECANVAS_H
#define BEZIERCURVECANVAS_H

#include "basecanvaswidget.h"

class BezierCurveCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit BezierCurveCanvas(QWidget *parent = nullptr);
    ~BezierCurveCanvas();
    
    void toggleCurveVisibility(bool visible) { showCurve = visible; update(); }
    void toggleControlPolygon(bool visible) { showControlPolygon = visible; update(); }
    
    // 重写基类方法
    void clearPoints() override;
    void drawCurves(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void deletePoint(int index) override;

protected:
    void drawHoverIndicator(QPainter &painter) override;
    double distance(const QPointF &p1, const QPointF &p2);

private:
    QVector<QPointF> calculateBezierCurve();
    QPointF bezierPoint(double t);
    
    QVector<QPointF> bezierPoints;
    QVector<double> weights;  // 存储每个点的权重
    bool showControlPolygon = true;
};

#endif // BEZIERCURVECANVAS_H