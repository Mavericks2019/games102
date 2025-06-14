#ifndef BSPLINECANVAS_H
#define BSPLINECANVAS_H

#include "basecanvaswidget.h"
#include <vector>

class BSplineCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit BSplineCanvas(QWidget *parent = nullptr);
    
    void setDegree(int degree);
    void toggleCurveVisibility(bool visible) { showCurve = visible; update(); }
    void toggleControlPolygon(bool visible) { showControlPolygon = visible; update(); }
    
    // 重写基类方法
    void drawCurves(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;

private:
    QVector<QPointF> calculateBSpline();
    double basisFunction(int i, int k, double t);
    void updateKnotVector()
    {
        int n = points.size() - 1;
        knots.clear();
        
        // 创建均匀节点向量
        int knotCount = n + degree + 2;
        for (int i = 0; i < knotCount; ++i) {
            knots.push_back(static_cast<double>(i) / (knotCount - 1));
        }
    };
    QVector<QPointF> bSplinePoints;
    std::vector<double> knots;
    int degree = 3;
    bool showControlPolygon = true;
};

#endif // BSPLINECANVAS_H