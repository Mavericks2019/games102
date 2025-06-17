#include "polygoncanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include <Eigen/Dense>
#include <iostream>

using namespace Eigen;

void PolygonCanvas::setAlpha(double a)
{
    alpha = a;
    update();
}

PolygonCanvas::PolygonCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    setMinimumSize(800, 600);
    setMouseTracking(true);
    curveColor = Qt::darkYellow;
}

void PolygonCanvas::clearPoints()
{
    BaseCanvasWidget::clearPoints();
    originalPolygon.clear();
    lastSubdivided.clear();
    subdivisionCount = 0;
    allowAddPoints = true; // 重置为允许添加点
    type = 0;
    emit subdivisionCountChanged(subdivisionCount);
}

void PolygonCanvas::setCurveType(CurveType type)
{
    curveType = type;
    update();
}

void PolygonCanvas::restoreOriginalPolygon()
{
    if (!originalPolygon.isEmpty()) {
        points.clear();
        for (const QPointF& pt : originalPolygon) {
            points.append({pt, false, true}); // 原始点可拖动
        }
        lastSubdivided.clear();
        subdivisionCount = 0;
        allowAddPoints = true; // 重置为允许添加点
        type = 0;
        emit subdivisionCountChanged(subdivisionCount);
        update();
    }
}

void PolygonCanvas::performChaikinSubdivision()
{
    if(type != 0 && type != 1) {
        return;
    }
    type = 1;
    if (points.size() < 2 || subdivisionCount >= MAX_SUBDIVISION) {
        return;
    }
    
    // 如果是第一次细分，保存原始多边形并禁止添加新点
    if (originalPolygon.isEmpty()) {
        originalPolygon.clear();
        for (const Point &pt : points) {
            originalPolygon.append(pt.pos);
        }
    }
    
    // 保存上一次的细分结果
    lastSubdivided.clear();
    for (const auto& pt : points) {
        lastSubdivided.append(pt.pos);
    }
    
    // 执行Chaikin细分
    QVector<Point> newPoints;
    int n = points.size();
    allowAddPoints = false; // 开始细分后禁止添加新点
    for (int i = 0; i < n; i++) {
        int next = (i + 1) % n;
        
        // 计算两个新点：1/4和3/4处
        QPointF q0 = QPointF(
            0.75 * points[i].pos.x() + 0.25 * points[next].pos.x(),
            0.75 * points[i].pos.y() + 0.25 * points[next].pos.y()
        );
        
        QPointF q1 = QPointF(
            0.25 * points[i].pos.x() + 0.75 * points[next].pos.x(),
            0.25 * points[i].pos.y() + 0.75 * points[next].pos.y()
        );
        
        // 细分生成的点不可拖动
        newPoints.append({q0, false, false});
        newPoints.append({q1, false, false});
    }
    
    points = newPoints;
    subdivisionCount++;
    emit subdivisionCountChanged(subdivisionCount);
    update();
}

void PolygonCanvas::performChaikincubedivision()
{
    if(type != 0 && type != 2) {
        return;
    }
    type = 2;
    if (points.size() < 2 || subdivisionCount >= MAX_SUBDIVISION) {
        return;
    }
    
    // 如果是第一次细分，保存原始多边形并禁止添加新点
    if (originalPolygon.isEmpty()) {
        originalPolygon.clear();
        for (const Point &pt : points) {
            originalPolygon.append(pt.pos);
        }
    }
    
    // 保存上一次的细分结果
    lastSubdivided.clear();
    for (const auto& pt : points) {
        lastSubdivided.append(pt.pos);
    }
    // 执行Chaikin细分
    QVector<Point> newPoints;
    int n = points.size();
    allowAddPoints = false; // 开始细分后禁止添加新点
    for (int i = 0; i < n; i++) {
        if(i == 0) {
            QPointF q0 = QPointF(
                0.125 * points[n - 1].pos.x() + 0.75 * points[i].pos.x() + 0.125 * points[i + 1].pos.x(),
                0.125 * points[n - 1].pos.y() + 0.75 * points[i].pos.y() + 0.125 * points[i + 1].pos.y()
            );
            QPointF q1 = QPointF(
                0.5 * points[i].pos.x() + 0.5 * points[i + 1].pos.x(),
                0.5 * points[i].pos.y() + 0.5 * points[i + 1].pos.y()
            );
            newPoints.append({q0, false, false});
            newPoints.append({q1, false, false});
        }else if(i == n - 1) {
            QPointF q0 = QPointF(
                0.125 * points[i - 1].pos.x() + 0.75 * points[i].pos.x() + 0.125 * points[0].pos.x(),
                0.125 * points[i - 1].pos.y() + 0.75 * points[i].pos.y() + 0.125 * points[0].pos.y()
            );
            
            QPointF q1 = QPointF(
                0.5 * points[i].pos.x() + 0.5 * points[0].pos.x(),
                0.5 * points[i].pos.y() + 0.5 * points[0].pos.y()
            );
            newPoints.append({q0, false, false});
            newPoints.append({q1, false, false});
        } else {
            QPointF q0 = QPointF(
                0.125 * points[i - 1].pos.x() + 0.75 * points[i].pos.x() + 0.125 * points[i + 1].pos.x(),
                0.125 * points[i - 1].pos.y() + 0.75 * points[i].pos.y() + 0.125 * points[i + 1].pos.y()
            );
            
            QPointF q1 = QPointF(
                0.5 * points[i].pos.x() + 0.5 * points[i + 1].pos.x(),
                0.5 * points[i].pos.y() + 0.5 * points[i + 1].pos.y()
            );
            newPoints.append({q0, false, false});
            newPoints.append({q1, false, false});
        }
    }
    
    points = newPoints;
    subdivisionCount++;
    emit subdivisionCountChanged(subdivisionCount);
    update();
}

void PolygonCanvas::performInterpolationdivision()
{
    if(type != 0 && type != 2) {
        return;
    }
    type = 2;
    if (points.size() < 2 || subdivisionCount >= MAX_SUBDIVISION) {
        return;
    }
    
    // 如果是第一次细分，保存原始多边形并禁止添加新点
    if (originalPolygon.isEmpty()) {
        originalPolygon.clear();
        for (const Point &pt : points) {
            originalPolygon.append(pt.pos);
        }
    }
    
    // 保存上一次的细分结果
    lastSubdivided.clear();
    for (const auto& pt : points) {
        lastSubdivided.append(pt.pos);
    }
    // 执行Chaikin细分
    QVector<Point> newPoints;
    int n = points.size();
    allowAddPoints = false; // 开始细分后禁止添加新点
    // 使用alpha值进行插值细分
    for (int i = 0; i < n; i++) {
        // 保留原始点
        Point originalPoint = points[i];
        originalPoint.movable = false; // 细分后点不可拖动
        newPoints.append(originalPoint);
        
        // 在原始点之间插入新点
        int prev = (i - 1 + n) % n;
        int next = (i + 1) % n;
        int nextNext = (i + 2) % n;
        
        // 使用alpha值计算新点位置
        std::cout << alpha << std::endl;
        double x = (-alpha / 2) * points[prev].pos.x() + 
                   (0.5 + alpha / 2) * points[i].pos.x() + 
                   (0.5 + alpha / 2) * points[next].pos.x() + 
                   (-alpha / 2) * points[nextNext].pos.x();
                  
        double y = (-alpha / 2) * points[prev].pos.y() + 
                   (0.5 + alpha / 2) * points[i].pos.y() + 
                   (0.5 + alpha / 2) * points[next].pos.y() + 
                   (-alpha / 2) * points[nextNext].pos.y();
        
        // 添加新点
        newPoints.append({QPointF(x, y), false, false});
    }
    
    points = newPoints;
    subdivisionCount++;
    emit subdivisionCountChanged(subdivisionCount);
    update();
}

QVector<QPointF> PolygonCanvas::calculateQuadraticSpline()
{
    QVector<QPointF> curve;
    int n = originalPolygon.size();
    if (n < 3) return curve; // 至少需要3个点
    
    // 创建扩展控制点序列：原始点 + 前2个点
    QVector<QPointF> extendedPoints = originalPolygon;
    extendedPoints.append(originalPolygon[0]);
    extendedPoints.append(originalPolygon[1]);
    
    // 计算所有曲线段（包括连接首尾的段）
    for (int i = 0; i < n; i++) {
        QPointF p0 = extendedPoints[i];
        QPointF p1 = extendedPoints[i+1];
        QPointF p2 = extendedPoints[i+2];
        
        for (double t = 0; t <= 1.0; t += 0.05) {
            double t2 = t * t;
            double b0 = (1 - 2*t + t2) / 2.0;
            double b1 = (1 + 2*t - 2*t2) / 2.0;
            double b2 = t2 / 2.0;
            
            double x = b0 * p0.x() + b1 * p1.x() + b2 * p2.x();
            double y = b0 * p0.y() + b1 * p1.y() + b2 * p2.y();
            curve.append(QPointF(x, y));
        }
    }
    
    return curve;
}

QVector<QPointF> PolygonCanvas::calculateCubicUniformBSpline()
{
    QVector<QPointF> curve;
    int n = originalPolygon.size();
    if (n < 4) return curve; // 至少需要4个点
    
    // 创建扩展控制点序列：原始点 + 前3个点
    QVector<QPointF> extendedPoints = originalPolygon;
    extendedPoints.append(originalPolygon[0]);
    extendedPoints.append(originalPolygon[1]);
    extendedPoints.append(originalPolygon[2]);
    
    // 计算所有曲线段（包括连接首尾的段）
    for (int i = 0; i < n; i++) {
        QPointF p0 = extendedPoints[i];
        QPointF p1 = extendedPoints[i+1];
        QPointF p2 = extendedPoints[i+2];
        QPointF p3 = extendedPoints[i+3];
        
        for (double t = 0; t <= 1.0; t += 0.05) {
            double t2 = t * t;
            double t3 = t2 * t;
            
            double b0 = (1 - 3*t + 3*t2 - t3) / 6.0;
            double b1 = (4 - 6*t2 + 3*t3) / 6.0;
            double b2 = (1 + 3*t + 3*t2 - 3*t3) / 6.0;
            double b3 = t3 / 6.0;
            
            double x = b0 * p0.x() + b1 * p1.x() + b2 * p2.x() + b3 * p3.x();
            double y = b0 * p0.y() + b1 * p1.y() + b2 * p2.y() + b3 * p3.y();
            curve.append(QPointF(x, y));
        }
    }
    
    return curve;
}

void PolygonCanvas::drawCurves(QPainter &painter)
{
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制原始多边形（虚线）
    if (!originalPolygon.isEmpty()) {
        painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        for (int i = 0; i < originalPolygon.size(); i++) {
            int next = (i + 1) % originalPolygon.size();
            painter.drawLine(originalPolygon[i], originalPolygon[next]);
        }
    }
    
    // 绘制上一次细分结果（虚线）
    if (!lastSubdivided.isEmpty() && lastSubdivided != originalPolygon) {
        painter.setPen(QPen(QColor(150, 150, 200), 1, Qt::DashLine));
        for (int i = 0; i < lastSubdivided.size(); i++) {
            int next = (i + 1) % lastSubdivided.size();
            painter.drawLine(lastSubdivided[i], lastSubdivided[next]);
        }
    }
    
    // 绘制当前多边形
    painter.setPen(QPen(curveColor, 2));
    for (int i = 0; i < points.size(); i++) {
        int next = (i + 1) % points.size();
        painter.drawLine(points[i].pos, points[next].pos);
    }
    
    // 绘制样条曲线 - 始终基于原始多边形点
    if (curveType != None && !originalPolygon.isEmpty()) {
        QVector<QPointF> splinePoints;
        
        switch (curveType) {
            case QuadraticSpline:
                if (originalPolygon.size() >= 3) {
                    splinePoints = calculateQuadraticSpline();
                    painter.setPen(QPen(Qt::red, 2));
                }
                break;
            case CubicUniformBSpline:
                if (originalPolygon.size() >= 4) {
                    splinePoints = calculateCubicUniformBSpline();
                    painter.setPen(QPen(Qt::blue, 2));
                }
                break;
            default:
                break;
        }
        
        if (!splinePoints.isEmpty()) {
            for (int i = 1; i < splinePoints.size(); ++i) {
                painter.drawLine(splinePoints[i-1], splinePoints[i]);
            }
        }
    }
}

void PolygonCanvas::drawPoints(QPainter &painter)
{
    painter.setPen(Qt::black);
    
    // 绘制原始多边形点（始终显示）
    for (int i = 0; i < originalPolygon.size(); ++i) {
        painter.setBrush(QColor(100, 200, 100)); // 绿色
        painter.drawEllipse(originalPolygon[i], 6, 6);
    }
    
    // 绘制当前点
    for (int i = 0; i < points.size(); ++i) {
        if (points[i].movable) {
            // 可拖动点
            if (i == hoveredIndex) {
                painter.setBrush(QColor(255, 100, 100)); // 悬停点
            } else {
                painter.setBrush(Qt::red); // 普通点
            }
        } else {
            // 不可拖动点（细分生成）
            painter.setBrush(QColor(150, 150, 200)); // 蓝灰色
        }
        painter.drawEllipse(points[i].pos, 6, 6);
    }
}

void PolygonCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    
    QString curveTypeStr;
    switch (curveType) {
        case None: curveTypeStr = "None"; break;
        case QuadraticSpline: curveTypeStr = "Quadratic Uniform B-Spline"; break;
        case CubicUniformBSpline: curveTypeStr = "Cubic Uniform B-Spline"; break;
    }
    
    QString info = "Chaikin Subdivision: " + QString::number(subdivisionCount) + "/6";
    info += " | Curve: " + curveTypeStr;
    if (!originalPolygon.isEmpty()) {
        info += " (Points: " + QString::number(originalPolygon.size()) + ")";
    }
    
    // 添加是否可以添加点的信息
    if (!allowAddPoints) {
        info += " | Adding points disabled";
    }
    
    painter.drawText(10, 20, info);
}

void PolygonCanvas::mousePressEvent(QMouseEvent *event)
{
    // 只允许选择可拖动的点
    if (event->button() == Qt::LeftButton) {
        selectedIndex = -1;
        hoveredIndex = -1;
        
        double minDist = 20.0;
        for (int i = 0; i < points.size(); ++i) {
            if (!points[i].movable) continue; // 跳过不可拖动的点
            
            double d = distance(event->pos(), points[i].pos);
            if (d < minDist) {
                minDist = d;
                selectedIndex = i;
                hoveredIndex = i;
            }
        }
        
        if (selectedIndex >= 0) {
            points[selectedIndex].moving = true;
            emit pointHovered(points[selectedIndex].pos);
        }
    }
    // 禁止右键添加点
    else if (event->button() == Qt::RightButton) {
        // 不调用基类实现，防止右键添加点
        return;
    }
}

void PolygonCanvas::mouseMoveEvent(QMouseEvent *event)
{
    // 更新悬停状态 - 只考虑可拖动的点
    hoveredIndex = -1;
    double minDist = 20.0;
    for (int i = 0; i < points.size(); ++i) {
        if (!points[i].movable) continue; // 跳过不可拖动的点
        
        double d = distance(event->pos(), points[i].pos);
        if (d < minDist) {
            minDist = d;
            hoveredIndex = i;
        }
    }
    
    if (hoveredIndex >= 0) {
        emit pointHovered(points[hoveredIndex].pos);
    } else {
        emit noPointHovered();
    }
    
    // 处理点拖动 - 只拖动可移动的点
    if ((event->buttons() & Qt::LeftButton) && selectedIndex >= 0) {
        points[selectedIndex].pos = event->pos();
        emit pointHovered(points[selectedIndex].pos);
        
        // 更新原始多边形点
        if (selectedIndex < originalPolygon.size()) {
            originalPolygon[selectedIndex] = points[selectedIndex].pos;
        }
        
        update();
    }
}

void PolygonCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selectedIndex >= 0) {
            points[selectedIndex].moving = false;
            selectedIndex = -1;
            
            if (!lastSubdivided.isEmpty()) {
                lastSubdivided.clear();
            }
        } else if (allowAddPoints) { // 只有在允许添加点时才能添加新点
            // 添加新点 - 新添加的点总是可拖动的
            Point newPoint = {event->pos(), false, true};
            points.append(newPoint);
            hoveredIndex = points.size() - 1;
            emit pointHovered(points.last().pos);
            
            lastSubdivided.clear();
            
            // 如果是第一次添加点，设置原始多边形
            if (originalPolygon.isEmpty()) {
                // 保存当前点为原始多边形
                for (const Point &pt : points) {
                    originalPolygon.append(pt.pos);
                }
            } else {
                // 添加新点到原始多边形
                originalPolygon.append(event->pos());
            }
        }
        update();
    }
}