// [文件]: canvas.h
#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <vector>
#include "control_point.h"

class Canvas : public QWidget {
    Q_OBJECT
public:
    // 曲线类型枚举
    enum CurveType {
        OriginalSpline,
        BezierCurve,
        QuadraticSpline,
        CubicSpline
    };
    
    Canvas(QWidget *parent = nullptr);
    void setShowCurve(bool show);
    void clearPoints();
    int getPointCount() const;
    void setCurveType(CurveType type);
    CurveType getCurveType() const { return curveType; }
    
signals:
    void pointHovered(const QPointF& point);
    void noPointHovered();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void drawGrid(QPainter &painter);
    void drawControlPoint(QPainter &painter, const ControlPoint &point, int index);
    void drawTangents(QPainter &painter, const ControlPoint &point);
    void drawCurve(QPainter &painter);
    void drawBezierCurve(QPainter &painter);
    void drawQuadraticSpline(QPainter &painter);
    void drawCubicSpline(QPainter &painter);
    void generateSpline(std::vector<QPointF>& curvePoints);
    void generateBezierControlPoints(std::vector<QPointF>& bezierPoints);
    double distance(const QPointF& p1, const QPointF& p2);
    QColor getCurveColor() const;
    
    std::vector<ControlPoint> controlPoints;
    bool showCurve;
    int draggingPoint;
    bool draggingLeftTangent;
    bool draggingRightTangent;
    int nextId;
    CurveType curveType; // 当前曲线类型
    int hoveredIndex = -1; // 悬停点的索引
};

#endif // CANVAS_H