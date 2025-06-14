#include "cubicsplinecanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <Eigen/Dense>
#include <cmath>

using namespace Eigen;

CubicSplineCanvas::CubicSplineCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::darkGreen;
}

void CubicSplineCanvas::deletePoint(int index)
{
    // 移除与被删除点关联的控制点
    for (int i = controlPoints.size() - 1; i >= 0; --i) {
        if (controlPoints[i].parentIndex == index) {
            controlPoints.remove(i);
        }
    }
    
    // 更新其他控制点的索引
    for (ControlPoint& cp : controlPoints) {
        if (cp.parentIndex > index) {
            cp.parentIndex--;
        }
    }
    
    // 调用基类删除点
    BaseCanvasWidget::deletePoint(index);
    
    // 更新控制点状态
    updateControlPoints();
    selectedControlPoint = -1;
}

void CubicSplineCanvas::drawCurves(QPainter &painter)
{
    if (points.size() < 2) return;
    calculateSpline();
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 2));
    
    for (int i = 1; i < splinePoints.size(); ++i) {
        painter.drawLine(splinePoints[i-1], splinePoints[i]);
    }
}

void CubicSplineCanvas::drawPoints(QPainter &painter)
{
    // 绘制原始点
    painter.setPen(Qt::black);
    for (int i = 0; i < points.size(); ++i) {
        if (i == hoveredIndex) {
            painter.setBrush(QColor(255, 100, 100));
        } else {
            painter.setBrush(Qt::red);
        }
        painter.drawEllipse(points[i].pos, 6, 6);
    }
    
    // 绘制控制点（切线点）
    if (showControlPoints) {
        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        for (int i = 0; i < controlPoints.size(); ++i) {
            const ControlPoint& cp = controlPoints[i];
            painter.setBrush(cp.isTangent ? Qt::blue : Qt::green);
            painter.drawEllipse(cp.pos, 4, 4);
            
            // 绘制与父点的连线
            if (cp.parentIndex != -1) {
                painter.drawLine(points[cp.parentIndex].pos, cp.pos);
            }
        }
    }
}

void CubicSplineCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, 20, "Cubic Spline (C2 Continuous)");
}

void CubicSplineCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        selectedIndex = -1;
        hoveredIndex = findHoveredPoint(event->pos());
        
        // 检查是否是控制点
        bool controlPointSelected = false;
        for (int i = 0; i < controlPoints.size(); ++i) {
            QPointF diff = event->pos() - controlPoints[i].pos;
            if (diff.manhattanLength() < 15) {
                selectedControlPoint = i;
                controlPointSelected = true;
                break;
            }
        }
        
        if (controlPointSelected) {
            // 选中控制点
            return;
        }
        
        if (hoveredIndex >= 0) {
            selectedIndex = hoveredIndex;
            points[selectedIndex].moving = true;
            emit pointHovered(points[selectedIndex].pos);
            
            // 显示该点的导数控制点
            showDerivatives(selectedIndex);
        }
    }
}

void CubicSplineCanvas::mouseMoveEvent(QMouseEvent *event)
{
    // 更新悬停状态
    int newHoveredIndex = findHoveredPoint(event->pos());
    if (newHoveredIndex != hoveredIndex) {
        hoveredIndex = newHoveredIndex;
        update();
        
        if (hoveredIndex >= 0) {
            emit pointHovered(points[hoveredIndex].pos);
        } else {
            emit noPointHovered();
        }
    }
    
    // 处理点拖动
    if ((event->buttons() & Qt::LeftButton)) {
        if (selectedIndex >= 0) {
            points[selectedIndex].pos = event->pos();
            emit pointHovered(points[selectedIndex].pos);
            // 更新导数控制点位置
            showDerivatives(selectedIndex);
            update();
        } else if (selectedControlPoint >= 0) {
            controlPoints[selectedControlPoint].pos = event->pos();
            update();
        }
    }
}

void CubicSplineCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selectedIndex >= 0) {
            points[selectedIndex].moving = false;
            selectedIndex = -1;
        } else if (selectedControlPoint >= 0) {
            selectedControlPoint = -1;
        } else {
            // 添加新点
            points.append({event->pos(), false});
            hoveredIndex = points.size() - 1;
            emit pointHovered(points.last().pos);
        }
        update();
    }
}

void CubicSplineCanvas::calculateSpline()
{
    splinePoints.clear();
    
    if (points.size() < 2) return;
    
    // 准备数据 - 使用数学坐标
    VectorXd x(points.size()), y(points.size());
    for (int i = 0; i < points.size(); ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // 收集导数信息
    VectorXd dx(points.size()), dy(points.size());
    dx.setZero();
    dy.setZero();
    
    for (const ControlPoint& cp : controlPoints) {
        if (cp.parentIndex >= 0 && cp.parentIndex < points.size()) {
            QPointF mathControlPoint = toMathCoords(cp.pos);
            QPointF mathParentPoint = toMathCoords(points[cp.parentIndex].pos);
            
            // 计算导数向量
            double derivX = mathControlPoint.x() - mathParentPoint.x();
            double derivY = mathControlPoint.y() - mathParentPoint.y();
            
            // 根据切线方向应用导数
            if (cp.leftTangent) {
                dx(cp.parentIndex) += derivX;
                dy(cp.parentIndex) += derivY;
            } else {
                dx(cp.parentIndex) += derivX;
                dy(cp.parentIndex) += derivY;
            }
        }
    }
    
    // 使用Hermite插值替代自然样条
    for (int i = 0; i < points.size() - 1; ++i) {
        double x0 = x(i);
        double y0 = y(i);
        double x1 = x(i+1);
        double y1 = y(i+1);
        
        double dx0 = dx(i);
        double dy0 = dy(i);
        double dx1 = dx(i+1);
        double dy1 = dy(i+1);
        
        // Hermite插值参数
        double a_x = 2*x0 - 2*x1 + dx0 + dx1;
        double b_x = -3*x0 + 3*x1 - 2*dx0 - dx1;
        double c_x = dx0;
        double d_x = x0;
        
        double a_y = 2*y0 - 2*y1 + dy0 + dy1;
        double b_y = -3*y0 + 3*y1 - 2*dy0 - dy1;
        double c_y = dy0;
        double d_y = y0;
        
        // 生成曲线段
        for (double t = 0; t <= 1.0; t += 0.01) {
            double t2 = t*t;
            double t3 = t2*t;
            
            double px = a_x*t3 + b_x*t2 + c_x*t + d_x;
            double py = a_y*t3 + b_y*t2 + c_y*t + d_y;
            
            splinePoints.append(toScreenCoords(QPointF(px, py)));
        }
    }
}

// 添加新函数
void CubicSplineCanvas::updateControlPoints()
{
    // 移除无效的控制点
    for (int i = controlPoints.size() - 1; i >= 0; --i) {
        int parentIdx = controlPoints[i].parentIndex;
        if (parentIdx < 0 || parentIdx >= points.size()) {
            controlPoints.remove(i);
        }
    }
    
    // 更新控制点位置
    for (ControlPoint& cp : controlPoints) {
        if (cp.parentIndex >= 0 && cp.parentIndex < points.size()) {
            QPointF basePos = points[cp.parentIndex].pos;
            QPointF offset = cp.pos - basePos;
            // 限制偏移量在合理范围内
            if (offset.manhattanLength() > 150) {
                offset = offset * (150 / offset.manhattanLength());
            }
            cp.pos = basePos + offset;
        }
    }
}

void CubicSplineCanvas::showDerivatives(int pointIndex)
{
    // 生成或更新该点的导数控制点
    // 这里仅作为示例，实际实现需要计算导数
    
    // 移除旧的控制点
    for (int i = controlPoints.size() - 1; i >= 0; --i) {
        if (controlPoints[i].parentIndex == pointIndex) {
            controlPoints.remove(i);
        }
    }
    
    // 添加新的控制点（左右导数）
    if (pointIndex > 0) {
        // 左导数
        ControlPoint left;
        left.parentIndex = pointIndex;
        left.isTangent = true;
        left.leftTangent = true;
        left.pos = points[pointIndex].pos + QPointF(-30, -30);
        controlPoints.append(left);
    }
    
    if (pointIndex < points.size() - 1) {
        // 右导数
        ControlPoint right;
        right.parentIndex = pointIndex;
        right.isTangent = true;
        right.leftTangent = false;
        right.pos = points[pointIndex].pos + QPointF(30, 30);
        controlPoints.append(right);
    }
    
    update();
}