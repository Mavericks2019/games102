#include "bsplinecanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

BSplineCanvas::BSplineCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::darkCyan;
    // 初始化节点向量
    BSplineCanvas::updateKnotVector();
}

void BSplineCanvas::setDegree(int newDegree)
{
    degree = newDegree;
    BSplineCanvas::updateKnotVector();
    update();
}

void BSplineCanvas::drawCurves(QPainter &painter)
{
    if (points.size() <= degree) return;
    
    bSplinePoints = calculateBSpline();
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 2));
    
    for (int i = 1; i < bSplinePoints.size(); ++i) {
        painter.drawLine(bSplinePoints[i-1], bSplinePoints[i]);
    }
}

void BSplineCanvas::drawPoints(QPainter &painter)
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

void BSplineCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, 20, "B-Spline - Degree: " + QString::number(degree));
}


QVector<QPointF> BSplineCanvas::calculateBSpline()
{
    QVector<QPointF> curve;
    
    if (points.size() <= degree) return curve;
    
    // 更新节点向量
     BSplineCanvas::updateKnotVector();
    
    // 计算曲线上的点
    double tStart = knots[degree];
    double tEnd = knots[knots.size() - degree - 1];
    double step = (tEnd - tStart) / 100.0;
    
    for (double t = tStart; t <= tEnd; t += step) {
        double x = 0, y = 0;
        double sum = 0;
        
        for (int i = 0; i < points.size(); ++i) {
            double basis = basisFunction(i, degree, t);
            x += points[i].pos.x() * basis;
            y += points[i].pos.y() * basis;
            sum += basis;
        }
        
        // 如果基函数和不为1，则进行归一化（对于非均匀B样条通常不需要，但这里确保数值稳定性）
        if (sum != 0) {
            x /= sum;
            y /= sum;
        }
        
        curve.append(QPointF(x, y));
    }
    
    return curve;
}

double BSplineCanvas::basisFunction(int i, int k, double t)
{
    // Cox-de Boor递归公式
    if (k == 0) {
        if (t >= knots[i] && t < knots[i+1]) {
            return 1.0;
        }
        return 0.0;
    }
    
    double denom1 = knots[i+k] - knots[i];
    double denom2 = knots[i+k+1] - knots[i+1];
    
    double term1 = 0, term2 = 0;
    
    if (denom1 != 0) {
        term1 = ((t - knots[i]) / denom1) * basisFunction(i, k-1, t);
    }
    
    if (denom2 != 0) {
        term2 = ((knots[i+k+1] - t) / denom2) * basisFunction(i+1, k-1, t);
    }
    
    return term1 + term2;
}