// [file name]: polygoncanvas.h
#ifndef POLYGONCANVAS_H
#define POLYGONCANVAS_H

#include "basecanvaswidget.h"

class PolygonCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    enum CurveType {
        None,
        QuadraticSpline,
        CubicUniformBSpline
    };
    
    explicit PolygonCanvas(QWidget *parent = nullptr);
    
    void performChaikinSubdivision();
    void performChaikincubedivision();
    void performInterpolationdivision();
    void restoreOriginalPolygon();
    void clearPoints() override;
    void setCurveType(CurveType type);
    void setAlpha(double a);
    int getSubdivisionCount() const { return subdivisionCount; }
    
    // 重写基类方法
    void drawCurves(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    double distance(const QPointF &p1, const QPointF &p2)
    {
        double dx = p1.x() - p2.x();
        double dy = p1.y() - p2.y();
        return std::sqrt(dx*dx + dy*dy);
    }
signals:
    void subdivisionCountChanged(int count);
    
private:
    QVector<QPointF> calculateQuadraticSpline();
    QVector<QPointF> calculateCubicUniformBSpline();
    QVector<QPointF> originalPolygon;
    QVector<QPointF> lastSubdivided;
    CurveType curveType = None;
    int subdivisionCount = 0;
    const int MAX_SUBDIVISION = 10;
    bool allowAddPoints = true;
    int type = 0;
    double alpha = 0.05;
};

#endif // POLYGONCANVAS_H