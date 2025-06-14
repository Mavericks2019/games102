#include "beziercurvecanvas.h"
#include <QPainter>
#include <QPointF>
#include <QMouseEvent>
#include <vector>
#include <math.h>

BezierCurveCanvas::BezierCurveCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::magenta;
}

void BezierCurveCanvas::drawCurves(QPainter &painter)
{
    if (points.size() < 2) return;
    
    bezierPoints = calculateBezierCurve();
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 2));
    
    for (int i = 1; i < bezierPoints.size(); ++i) {
        painter.drawLine(bezierPoints[i-1], bezierPoints[i]);
    }
}

void BezierCurveCanvas::drawPoints(QPainter &painter)
{
    // 绘制控制点
    painter.setPen(Qt::black);
    for (int i = 0; i < points.size(); ++i) {
        if (i == hoveredIndex) {
            painter.setBrush(QColor(255, 100, 100));
        } else {
            painter.setBrush(Qt::red);
        }
        painter.drawEllipse(points[i].pos, 6, 6);
    }
    
    // 绘制控制多边形
    if (showControlPolygon) {
        painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        for (int i = 1; i < points.size(); ++i) {
            painter.drawLine(points[i-1].pos, points[i].pos);
        }
    }
}

void BezierCurveCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, 20, "Bezier Curve - Degree: " + QString::number(points.size()-1));
}

QVector<QPointF> BezierCurveCanvas::calculateBezierCurve()
{
    QVector<QPointF> curve;
    
    if (points.size() < 2) return curve;
    
    for (double t = 0; t <= 1.0; t += 0.005) {
        curve.append(bezierPoint(t));
    }
    
    return curve;
}

QPointF BezierCurveCanvas::bezierPoint(double t)
{
    int n = points.size() - 1;
    if (n < 1) return QPointF();
    
    std::vector<double> coeffs(n+1, 0);
    double one_minus_t = 1.0 - t;
    
    // 计算二项式系数
    std::vector<double> binomial(n+1, 0);
    binomial[0] = 1.0;
    for (int i = 1; i <= n; ++i) {
        binomial[i] = binomial[i-1] * (n - i + 1) / i;
    }
    
    double x = 0, y = 0;
    for (int i = 0; i <= n; ++i) {
        double blend = binomial[i] * pow(one_minus_t, n-i) * pow(t, i);
        x += points[i].pos.x() * blend;
        y += points[i].pos.y() * blend;
    }
    
    return QPointF(x, y);
}