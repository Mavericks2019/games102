#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <vector>
#include "control_point.h"

class Canvas : public QWidget {
    Q_OBJECT
public:
    // 公有成员变量
    QColor curveColor;
    
    Canvas(QWidget *parent = nullptr);
    void setShowCurve(bool show);
    void clearPoints();
    int getPointCount() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void drawGrid(QPainter &painter);
    void drawControlPoint(QPainter &painter, const ControlPoint &point, int index);
    void drawTangents(QPainter &painter, const ControlPoint &point);
    void drawCurve(QPainter &painter);
    void generateSpline(std::vector<QPointF>& curvePoints);
    double distance(const QPointF& p1, const QPointF& p2);
    void solveTridiagonalSystem(std::vector<double>& a, std::vector<double>& b, 
                                  std::vector<double>& c, std::vector<double>& d, 
                                  std::vector<double>& x);
    std::vector<ControlPoint> controlPoints;
    bool showCurve;
    int draggingPoint;
    bool draggingLeftTangent;
    bool draggingRightTangent;
    int nextId;
};

#endif // CANVAS_H