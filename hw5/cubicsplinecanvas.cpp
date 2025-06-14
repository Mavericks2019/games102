#include "cubicsplinecanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>

using namespace Eigen;

CubicSplineCanvas::CubicSplineCanvas(QWidget *parent)
    : BaseCanvasWidget(parent)
{
    curveColor = Qt::darkGreen;
    selectedIndex = -1;
    hoveredIndex = -1;
    draggingLeftTangent = false;
    draggingRightTangent = false;
}

// 添加clearPoints实现
void CubicSplineCanvas::clearPoints()
{
    points.clear();
    splinePoints.clear();
    selectedIndex = -1;
    hoveredIndex = -1;
    draggingLeftTangent = false;
    draggingRightTangent = false;
    update();
    emit noPointHovered();
}

void CubicSplineCanvas::drawCurves(QPainter &painter)
{
    if (points.size() < 2) return;
    
    calculateSplineNaive();
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(curveColor, 2));
    
    for (int i = 1; i < splinePoints.size(); ++i) {
        painter.drawLine(splinePoints[i-1], splinePoints[i]);
    }
}

void CubicSplineCanvas::drawPoints(QPainter &painter)
{
    // 绘制原始点
    for (int i = 0; i < points.size(); ++i) {
        const auto& point = points[i];
        painter.setPen(Qt::black);
        // 设置点的颜色
        QColor pointColor;
        if (i == hoveredIndex) {
            pointColor = QColor(255, 100, 100); // 悬停时为浅红色
        } else if (point.selected) {
            pointColor = QColor(241, 196, 15); // 选中时为黄色
        } else {
            pointColor = QColor(Qt::red); // 普通时为红
        }
        
        // 绘制点
        painter.setBrush(pointColor);
        painter.setPen(Qt::black); // 白色边框
        painter.drawEllipse(point.pos, 6, 6);
        
        // 绘制点编号
        painter.setPen(QColor(0, 0, 0)); // 黑色文本
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(point.pos + QPointF(12, -12), QString::number(i));
        
        // 只有当点被选中时才绘制其切线控制点
        if (point.selected && showControlPoints) {
            // 绘制左切线（如果不是第一个点）
            if (i > 0) {
                QPointF leftEnd = point.pos + point.leftTangent;
                
                // 绘制切线
                QColor leftTangentColor = point.leftTangentFixed ? 
                    QColor(46, 204, 113) : QColor(155, 89, 182); // 固定为绿色，否则紫色
                painter.setPen(QPen(leftTangentColor, 2));
                painter.drawLine(point.pos, leftEnd);
                
                // 绘制切线控制点（矩形）
                painter.setBrush(QColor(231, 76, 60)); // 红色填充
                painter.setPen(QPen(QColor(236, 240, 241), 1)); // 白色边框
                painter.drawRect(leftEnd.x() - 6, leftEnd.y() - 6, 12, 12);
            }
            
            // 绘制右切线（如果不是最后一个点）
            if (i < points.size() - 1) {
                QPointF rightEnd = point.pos + point.rightTangent;
                
                // 绘制切线
                QColor rightTangentColor = point.rightTangentFixed ? 
                    QColor(46, 204, 113) : QColor(155, 89, 182); // 固定为绿色，否则紫色
                painter.setPen(QPen(rightTangentColor, 2));
                painter.drawLine(point.pos, rightEnd);
                
                // 绘制切线控制点（矩形）
                painter.setBrush(QColor(231, 76, 60)); // 红色填充
                painter.setPen(QPen(QColor(236, 240, 241), 1)); // 白色边框
                painter.drawRect(rightEnd.x() - 6, rightEnd.y() - 6, 12, 12);
            }
        }
    }
}

void CubicSplineCanvas::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, 20, "Cubic Spline - Tangent points visible only when selected");
}

void CubicSplineCanvas::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = event->pos();
    
    if (event->button() == Qt::LeftButton) {
        selectedIndex = -1;
        draggingLeftTangent = false;
        draggingRightTangent = false;
        
        // 1. 首先检查是否点击了任何选中点的切线端点
        for (int i = 0; i < points.size(); ++i) {
            // 只检查已选中点的切线端点
            if (points[i].selected) {
                // 检查左切线端点（只有不是第一个点时才有左切线）
                if (i > 0) {
                    QPointF leftEnd = points[i].pos + points[i].leftTangent;
                    if (distance(pos, leftEnd) < 10) {
                        selectedIndex = i;
                        draggingLeftTangent = true;
                        points[i].leftTangentFixed = true;
                        break;
                    }
                }
                
                // 检查右切线端点（只有不是最后一个点时才有右切线）
                if (i < points.size() - 1) {
                    QPointF rightEnd = points[i].pos + points[i].rightTangent;
                    if (distance(pos, rightEnd) < 10) {
                        selectedIndex = i;
                        draggingRightTangent = true;
                        points[i].rightTangentFixed = true;
                        break;
                    }
                }
            }
        }
        
        // 2. 如果没有点击切线端点，检查是否点击了原始点
        if (selectedIndex == -1) {
            double minDist = 20.0;
            for (int i = 0; i < points.size(); ++i) {
                double d = distance(pos, points[i].pos);
                if (d < minDist) {
                    minDist = d;
                    selectedIndex = i;
                    hoveredIndex = i;
                }
            }
            
            if (selectedIndex >= 0) {
                // 清除所有点的选中状态
                for (auto& point : points) {
                    point.selected = false;
                }
                
                // 选中当前点
                points[selectedIndex].moving = true;
                points[selectedIndex].selected = true;
                emit pointHovered(points[selectedIndex].pos);
            }
        }
    } else if (event->button() == Qt::RightButton) {
        // 右键删除点
        int indexToDelete = -1;
        double minDist = 20.0;
        
        // 查找最近的原始点
        for (int i = 0; i < points.size(); ++i) {
            double d = distance(event->pos(), points[i].pos);
            if (d < minDist) {
                minDist = d;
                indexToDelete = i;
            }
        }
        
        if (indexToDelete != -1) {
            // 删除点
            updateAdjacentTangents(indexToDelete);
            points.remove(indexToDelete);
        } else if (!points.isEmpty()) {
            // 如果右键点击空白处，删除最后一个点
            indexToDelete = points.size() - 1;
            updateAdjacentTangents(indexToDelete);
            points.removeLast();
        }
        
        selectedIndex = -1;
        hoveredIndex = -1;
        update();
    }
}

void CubicSplineCanvas::updateAdjacentTangents(int index)
{
    // 更新前一个点的切线（如果存在）
    if (index > 0 && !points[index-1].rightTangentFixed) {
        if (index < points.size() - 1) {
            // 计算从前一个点到后一个点的方向
            QPointF dir = points[index+1].pos - points[index-1].pos;
            qreal length = distance(points[index-1].pos, points[index+1].pos);
            if (length > 0.001) {
                dir = dir * (0.3 * length / length);
            } else {
                dir = QPointF(20, 0);
            }
            points[index-1].rightTangent = dir;
        } else {
            // 如果是最后一个点，重置前一个点的右切线
            points[index-1].rightTangent = QPointF(20, 0);
        }
    }
    
    // 更新后一个点的切线（如果存在）
    if (index < points.size() - 1 && !points[index+1].leftTangentFixed) {
        if (index > 0) {
            // 计算从后一个点到前一个点的方向
            QPointF dir = points[index-1].pos - points[index+1].pos;
            qreal length = distance(points[index+1].pos, points[index-1].pos);
            if (length > 0.001) {
                dir = dir * (0.3 * length / length);
            } else {
                dir = QPointF(-20, 0);
            }
            points[index+1].leftTangent = dir;
        } else {
            // 如果是第一个点，重置后一个点的左切线
            points[index+1].leftTangent = QPointF(-20, 0);
        }
    }
}

void CubicSplineCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPointF pos = event->pos();
    
    // 更新悬停状态
    hoveredIndex = -1;
    double minDist = 20.0;
    for (int i = 0; i < points.size(); ++i) {
        double d = distance(pos, points[i].pos);
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
    
    // 处理点拖动
    if (event->buttons() & Qt::LeftButton) {
        if (selectedIndex >= 0) {
            if (draggingLeftTangent) {
                // 拖动左切线
                points[selectedIndex].leftTangent = pos - points[selectedIndex].pos;
            } else if (draggingRightTangent) {
                // 拖动右切线
                points[selectedIndex].rightTangent = pos - points[selectedIndex].pos;
            } else {
                // 拖动原始点
                points[selectedIndex].pos = pos;
            }
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
            draggingLeftTangent = false;
            draggingRightTangent = false;
        } else {
            // 添加新点
            ControlPoint newPoint;
            newPoint.pos = event->pos();
            
            // 设置合理的切线方向
            if (!points.empty()) {
                // 获取上一个点
                ControlPoint& prevPoint = points.last();
                
                // 计算从上一个点到新点的距离
                qreal dist = distance(newPoint.pos, prevPoint.pos);
                
                // 设置合理的切线长度（距离的30%）
                qreal tangentLength = dist * 0.3;
                if (tangentLength < 5) tangentLength = 5; // 最小长度
                
                // 计算差值向量
                QPointF diffVector = newPoint.pos - prevPoint.pos;
                if (dist > 0.001) {
                    diffVector = diffVector * (tangentLength / dist);
                } else {
                    diffVector = QPointF(tangentLength, 0); // 默认方向
                }
                
                // 更新上一个点的右导数
                prevPoint.rightTangent = diffVector;
                
                // 设置新点的左侧切线（指向上一个点）
                newPoint.leftTangent = -diffVector;

                if (points.size() >= 2) {
                    // 获取上上个点
                    ControlPoint& prevPrevPoint = points[points.size()-2];
                    
                    // 计算从上上个点到新点的方向
                    QPointF globalDir = newPoint.pos - prevPrevPoint.pos;
                    qreal globalDist = distance(prevPrevPoint.pos, newPoint.pos);
                    
                    if (globalDist > 0.001) {
                        // 计算平滑的切线长度（全局距离的15%）
                        qreal smoothTangentLength = globalDist * 0.15;
                        
                        // 计算平滑的切线方向
                        QPointF smoothTangent = globalDir * (smoothTangentLength / globalDist);
                        
                        // 更新上一个点的左切线和右切线
                        prevPoint.leftTangent = -smoothTangent;
                        prevPoint.rightTangent = smoothTangent;
                    }
                }
            }
            
            // 清除所有点的选中状态
            for (auto& point : points) {
                point.selected = false;
            }
            
            // 选中新添加的点
            newPoint.selected = true;
            points.append(newPoint);
            selectedIndex = points.size() - 1;
            hoveredIndex = points.size() - 1;
            emit pointHovered(newPoint.pos);
        }
        update();
    }
}

void CubicSplineCanvas::calculateSpline()
{
    splinePoints.clear();
    
    if (points.size() < 2) return;
    
    // 使用三次贝塞尔曲线生成样条
    for (int i = 0; i < points.size() - 1; ++i) {
        const auto& p0 = points[i].pos;
        const auto& p3 = points[i+1].pos;
        
        QPointF p1, p2;
        
        // 计算起点控制点
        if (points[i].rightTangentFixed) {
            p1 = p0 + points[i].rightTangent;
        } else {
            if (i == 0) {
                // 第一个点：使用到下一个点的方向
                QPointF dir = p3 - p0;
                qreal length = distance(p0, p3);
                if (length > 0.001) {
                    dir = dir * (0.3 * length / length);
                } else {
                    dir = QPointF(20, 0);
                }
                p1 = p0 + dir;
            } else {
                // 使用前一个点和下一个点的平均方向
                QPointF prevDir = p0 - points[i-1].pos;
                QPointF nextDir = p3 - p0;
                
                qreal prevLength = distance(points[i-1].pos, p0);
                qreal nextLength = distance(p0, p3);
                
                // 计算加权平均方向
                QPointF avgDir = (prevDir * nextLength + nextDir * prevLength) / (prevLength + nextLength + 0.0001);
                qreal avgLength = (prevLength + nextLength) * 0.15;
                
                if (avgLength > 0.001) {
                    qreal avgDirLength = distance(QPointF(0,0), avgDir);
                    p1 = p0 + avgDir * (avgLength / (avgDirLength + 0.0001));
                } else {
                    p1 = p0 + QPointF(20, 0);
                }
            }
        }
        
        // 计算终点控制点
        if (points[i+1].leftTangentFixed) {
            p2 = p3 + points[i+1].leftTangent;
        } else {
            if (i == points.size() - 2) {
                // 最后一个点：使用到前一个点的方向
                QPointF dir = p0 - p3;
                qreal length = distance(p3, p0);
                if (length > 0.001) {
                    dir = dir * (0.3 * length / length);
                } else {
                    dir = QPointF(-20, 0);
                }
                p2 = p3 + dir;
            } else {
                // 使用当前点和下下个点的方向
                QPointF prevDir = p3 - p0;
                QPointF nextDir = points[i+2].pos - p3;
                
                qreal prevLength = distance(p0, p3);
                qreal nextLength = distance(p3, points[i+2].pos);
                
                // 计算加权平均方向
                QPointF avgDir = (prevDir * nextLength + nextDir * prevLength) / (prevLength + nextLength + 0.0001);
                qreal avgLength = (prevLength + nextLength) * 0.15;
                
                if (avgLength > 0.001) {
                    qreal avgDirLength = distance(QPointF(0,0), avgDir);
                    p2 = p3 + avgDir * (avgLength / (avgDirLength + 0.0001));
                } else {
                    p2 = p3 + QPointF(-20, 0);
                }
            }
        }
        
        // 生成贝塞尔曲线点
        for (double t = 0; t <= 1.0; t += 0.01) {
            double u = 1 - t;
            double x = u*u*u*p0.x() + 3*u*u*t*p1.x() + 3*u*t*t*p2.x() + t*t*t*p3.x();
            double y = u*u*u*p0.y() + 3*u*u*t*p1.y() + 3*u*t*t*p2.y() + t*t*t*p3.y();
            splinePoints.append(QPointF(x, y));
        }
    }
}

void CubicSplineCanvas::calculateSplineNaive() {
    splinePoints.clear();
    
    if (points.size() < 2) return;
    
    
    // 简单的三次样条实现
    for (size_t i = 0; i < points.size() - 1; ++i) {
        QPointF p0 = points[i].pos;
        QPointF p3 = points[i+1].pos;
        
        // 控制点（使用切线方向）
        QPointF p1 = p0 + (i > 0 ? (points[i].rightTangent) : QPointF(30, 0));
        QPointF p2 = p3 + (i < points.size()-2 ? points[i+1].leftTangent : QPointF(-30, 0));
        
        // 绘制三次贝塞尔曲线
        QPointF prev = p0;
        for (double t = 0.1; t <= 1.0; t += 0.005) {
            double u = 1 - t;
            double x = u*u*u*p0.x() + 3*u*u*t*p1.x() + 3*u*t*t*p2.x() + t*t*t*p3.x();
            double y = u*u*u*p0.y() + 3*u*u*t*p1.y() + 3*u*t*t*p2.y() + t*t*t*p3.y();
            QPointF p(x, y);
            splinePoints.append(QPointF(x, y));
            prev = p;
        }
    }
}

double CubicSplineCanvas::distance(const QPointF &p1, const QPointF &p2)
{
    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    return std::sqrt(dx*dx + dy*dy);
}