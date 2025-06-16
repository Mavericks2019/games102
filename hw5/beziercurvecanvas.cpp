#include "beziercurvecanvas.h"
#include <QPainter>
#include <QPointF>
#include <QMouseEvent>
#include <QWheelEvent>
#include <vector>
#include <math.h>
#include <QDebug>

BezierCurveCanvas::BezierCurveCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::magenta;
}

BezierCurveCanvas::~BezierCurveCanvas()
{
    // 确保清除所有权重
    weights.clear();
}

void BezierCurveCanvas::clearPoints()
{
    BaseCanvasWidget::clearPoints();
    weights.clear(); // 清除所有权重
    bezierPoints.clear();
}

void BezierCurveCanvas::drawCurves(QPainter &painter)
{
    if (points.size() < 2) return;
    
    bezierPoints = calculateBezierCurve();
    
    if (bezierPoints.isEmpty()) return;
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 2));
    
    for (int i = 1; i < bezierPoints.size(); ++i) {
        painter.drawLine(bezierPoints[i-1], bezierPoints[i]);
    }
    
    // 检查首尾点是否接近（形成闭合多边形）
    if (points.size() >= 3) {
        double dist = distance(points.first().pos, points.last().pos);
        if (dist < 15.0) {  // 15像素内视为闭合
            painter.drawLine(bezierPoints.last(), bezierPoints.first());
        }
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
        
        // 在点旁边绘制权重值
        if (i < weights.size()) {
            painter.setPen(Qt::darkBlue);
            painter.setFont(QFont("Arial", 8));
            QString weightText = QString::number(weights[i], 'f', 1);
            painter.drawText(points[i].pos + QPointF(10, -5), weightText);
        }
    }
    
    // 绘制控制多边形
    if (showControlPolygon) {
        painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        for (int i = 1; i < points.size(); ++i) {
            painter.drawLine(points[i-1].pos, points[i].pos);
        }
        
        // 如果首尾点接近，闭合多边形
        if (points.size() >= 3) {
            double dist = distance(points.first().pos, points.last().pos);
            if (dist < 15.0) {
                painter.drawLine(points.last().pos, points.first().pos);
            }
        }
    }
}

void BezierCurveCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    
    // 添加权重信息
    QString weightInfo = "Weights: ";
    for (int i = 0; i < weights.size(); ++i) {
        weightInfo += QString::number(weights[i], 'f', 1);
        if (i < weights.size() - 1) weightInfo += ", ";
    }
    
    painter.drawText(10, 20, "Bezier Curve - Degree: " + QString::number(points.size()-1));
    painter.drawText(10, 40, weightInfo);
}

void BezierCurveCanvas::drawHoverIndicator(QPainter &painter)
{
    if (hoveredIndex < 0 || hoveredIndex >= points.size() || hoveredIndex >= weights.size()) 
        return;
        
    const QPointF &p = points[hoveredIndex].pos;
    
    // 扩大文本背景以包含权重
    QRectF textRect(p.x() + 15, p.y() - 40, 120, 35);
    painter.setBrush(QColor(255, 255, 220, 220));
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.drawRoundedRect(textRect, 5, 5);
    
    // 添加权重信息
    QPointF mathPoint = toMathCoords(p);
    QString coordText = QString("(%1, %2)\nWeight: %3")
                            .arg(mathPoint.x(), 0, 'f', 1)
                            .arg(mathPoint.y(), 0, 'f', 1)
                            .arg(weights[hoveredIndex], 0, 'f', 1);
    
    painter.setPen(Qt::black);
    painter.drawText(textRect, Qt::AlignCenter, coordText);
    
    // 连接线
    painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    painter.drawLine(p, QPointF(p.x() + 15, p.y() - 20));
}

void BezierCurveCanvas::wheelEvent(QWheelEvent *event)
{
    if (hoveredIndex >= 0 && hoveredIndex < points.size() && hoveredIndex < weights.size()) {
        // 获取滚轮转动的角度，每15度为一个步进
        double delta = event->angleDelta().y() / 120.0;
        
        // 调整权重，步长0.1
        double step = 0.1;
        weights[hoveredIndex] += delta * step;
        
        // 权重不能为负
        if (weights[hoveredIndex] < 0.1) {
            weights[hoveredIndex] = 0.1;
        }
        
        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void BezierCurveCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selectedIndex >= 0) {
            points[selectedIndex].moving = false;
            selectedIndex = -1;
        } else {
            // 添加新点 - 新添加的点总是可拖动的
            Point newPoint = {event->pos(), false, true};
            points.append(newPoint);
            weights.append(1.0); // 默认权重为1.0
            hoveredIndex = points.size() - 1;
            emit pointHovered(points.last().pos);
        }
        update();
    }
}

void BezierCurveCanvas::deletePoint(int index)
{
    if (index < 0 || index >= points.size()) 
        return;
        
    points.remove(index);
    
    // 同步删除权重
    if (index < weights.size()) {
        weights.remove(index);
    }
    
    // 更新悬停索引
    if (hoveredIndex == index) {
        hoveredIndex = -1;
        emit noPointHovered();
    } else if (hoveredIndex > index) {
        hoveredIndex--;
    }
    
    // 更新选中索引
    if (selectedIndex == index) {
        selectedIndex = -1;
    } else if (selectedIndex > index) {
        selectedIndex--;
    }
    
    update();
    emit pointDeleted();
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
    
    // 确保权重向量与点向量大小一致
    if (weights.size() != points.size()) {
        qWarning() << "Weights and points size mismatch!";
        return QPointF();
    }
    
    std::vector<double> coeffs(n+1, 0);
    double one_minus_t = 1.0 - t;
    
    // 计算二项式系数
    std::vector<double> binomial(n+1, 0);
    binomial[0] = 1.0;
    for (int i = 1; i <= n; ++i) {
        binomial[i] = binomial[i-1] * (n - i + 1) / i;
    }
    
    // 计算有理贝塞尔曲线
    double x = 0, y = 0;
    double denominator = 0.0; // 分母，所有权重与基函数的乘积之和
    
    for (int i = 0; i <= n; ++i) {
        double blend = binomial[i] * pow(one_minus_t, n-i) * pow(t, i);
        denominator += weights[i] * blend;
    }
    
    // 避免除零
    if (denominator < 1e-10) {
        return points[0].pos; // 返回第一个点作为安全值
    }
    
    for (int i = 0; i <= n; ++i) {
        double blend = binomial[i] * pow(one_minus_t, n-i) * pow(t, i);
        double rationalBlend = weights[i] * blend / denominator;
        x += points[i].pos.x() * rationalBlend;
        y += points[i].pos.y() * rationalBlend;
    }
    
    return QPointF(x, y);
}

double BezierCurveCanvas::distance(const QPointF &p1, const QPointF &p2)
{
    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    return std::sqrt(dx*dx + dy*dy);
}