#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>
#include <QDebug>

ControlPoint::ControlPoint(QPointF p, int idx) : 
    pos(p), leftTangent(-20, 0), rightTangent(20, 0), 
    leftTangentFixed(false), rightTangentFixed(false), 
    selected(false), id(idx) {}

bool ControlPoint::operator==(const ControlPoint& other) const {
    return id == other.id;
}

Canvas::Canvas(QWidget *parent) : QWidget(parent), 
    showCurve(true), draggingPoint(-1), draggingLeftTangent(false),
    draggingRightTangent(false), nextId(0),
    curveType(OriginalSpline) {
    setMouseTracking(true);
    setStyleSheet("background-color: #2c313c;");
}

void Canvas::setCurveType(CurveType type) {
    curveType = type;
    update();
}

void Canvas::setShowCurve(bool show) { 
    showCurve = show; 
    update(); 
}

void Canvas::clearPoints() { 
    controlPoints.clear(); 
    draggingPoint = -1; 
    nextId = 0; 
    hoveredIndex = -1;
    update();
    emit noPointHovered();
}

int Canvas::getPointCount() const { 
    return controlPoints.size(); 
}

void Canvas::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 设置深色背景 - 与插值工具一致
    painter.fillRect(rect(), QColor(53, 53, 53));
    
    // 绘制网格
    drawGrid(painter);
    
    // 绘制控制点
    for (size_t i = 0; i < controlPoints.size(); i++) {
        drawControlPoint(painter, controlPoints[i], i);
    }
    
    // 绘制切线
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        if (controlPoints[i].selected) {
            drawTangents(painter, controlPoints[i]);
        }
    }
    
    // 根据曲线类型绘制曲线
    if (showCurve && controlPoints.size() >= 2) {
        switch (curveType) {
            case OriginalSpline:
                drawCurve(painter);
                break;
            case BezierCurve:
                drawBezierCurve(painter);
                break;
            case QuadraticSpline:
                drawQuadraticSpline(painter);
                break;
            case CubicSpline:
                drawCubicSpline(painter);
                break;
        }
    }
    
    // 绘制悬停指示器
    if (hoveredIndex >= 0 && hoveredIndex < static_cast<int>(controlPoints.size())) {
        const auto& point = controlPoints[hoveredIndex];
        
        // 绘制坐标文本背景
        QRectF textRect(point.pos.x() + 15, point.pos.y() - 30, 120, 25);
        painter.setBrush(QColor(255, 255, 220, 220));
        painter.setPen(QPen(Qt::darkGray, 1));
        painter.drawRoundedRect(textRect, 5, 5);
        
        // 绘制坐标文本
        QString coordText = QString("(%1, %2)")
            .arg(point.pos.x(), 0, 'f', 1)
            .arg(height() - point.pos.y(), 0, 'f', 1);
        
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 10));
        painter.drawText(textRect, Qt::AlignCenter, coordText);
        
        // 绘制连接线
        painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        painter.drawLine(point.pos, QPointF(point.pos.x() + 15, point.pos.y() - 15));
    }
}

void Canvas::drawGrid(QPainter &painter) {
    // 设置网格线颜色 - 与插值工具一致
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    
    int gridSize = 20;
    for (int x = 0; x < width(); x += gridSize) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += gridSize) {
        painter.drawLine(0, y, width(), y);
    }
}

void Canvas::drawControlPoint(QPainter &painter, const ControlPoint &point, int index) {
    // 绘制点
    QColor pointColor = (index == hoveredIndex) ? QColor(255, 100, 100) : // 悬停点红色
                     point.selected ? QColor(241, 196, 15) : QColor(52, 152, 219); // 普通点蓝色
    
    painter.setBrush(pointColor);
    painter.setPen(QPen(QColor(236, 240, 241), 2));
    painter.drawEllipse(point.pos, 8, 8);
    
    // 绘制点编号 - 与插值工具一致
    painter.setPen(QColor(236, 240, 241));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(point.pos + QPointF(12, -12), QString::number(index));
}
void Canvas::drawTangents(QPainter &painter, const ControlPoint &point) {
    QPointF leftEnd = point.pos + point.leftTangent;
    QPointF rightEnd = point.pos + point.rightTangent;
    
    // 绘制左侧切线
    QColor leftTangentColor = point.leftTangentFixed ? QColor(46, 204, 113) : QColor(155, 89, 182);
    painter.setPen(QPen(leftTangentColor, 2));
    painter.drawLine(point.pos, leftEnd);
    
    // 绘制左侧切线控制点
    painter.setBrush(QColor(231, 76, 60));
    painter.setPen(QPen(QColor(236, 240, 241), 1));
    painter.drawRect(leftEnd.x() - 6, leftEnd.y() - 6, 12, 12);
    
    // 绘制右侧切线
    QColor rightTangentColor = point.rightTangentFixed ? QColor(46, 204, 113) : QColor(155, 89, 182);
    painter.setPen(QPen(rightTangentColor, 2));
    painter.drawLine(point.pos, rightEnd);
    
    // 绘制右侧切线控制点
    painter.setBrush(QColor(231, 76, 60));
    painter.setPen(QPen(QColor(236, 240, 241), 1));
    painter.drawRect(rightEnd.x() - 6, rightEnd.y() - 6, 12, 12);
    
    // 绘制切线信息
    painter.setPen(QColor(236, 240, 241));
    painter.setFont(QFont("Arial", 9));
    painter.drawText(leftEnd + QPointF(15, 5), 
                     QString("Left: (%1, %2)")
                     .arg(point.leftTangent.x(), 0, 'f', 1)
                     .arg(point.leftTangent.y(), 0, 'f', 1));
    
    painter.drawText(rightEnd + QPointF(15, 5), 
                     QString("Right: (%1, %2)")
                     .arg(point.rightTangent.x(), 0, 'f', 1)
                     .arg(point.rightTangent.y(), 0, 'f', 1));
}

void Canvas::drawCurve(QPainter &painter) {
    std::vector<QPointF> curvePoints;
    generateSpline(curvePoints);
    
    if (curvePoints.size() < 2) return;
    painter.setRenderHint(QPainter::Antialiasing, true); // 增加抗锯齿
    // 使用当前曲线类型的颜色
    painter.setPen(QPen(getCurveColor(), 3));
    for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
        painter.drawLine(curvePoints[i], curvePoints[i+1]);
    }
}

void Canvas::drawBezierCurve(QPainter &painter) {
    // 绘制贝塞尔曲线
    std::vector<QPointF> curvePoints;
    generateSpline(curvePoints);
    
    if (curvePoints.size() < 2) return;
    painter.setRenderHint(QPainter::Antialiasing, true); // 增加抗锯齿
    // 使用当前曲线类型的颜色
    painter.setPen(QPen(getCurveColor(), 3));
    for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
        painter.drawLine(curvePoints[i], curvePoints[i+1]);
    }
    
    // 绘制贝塞尔控制点
    std::vector<QPointF> bezierPoints;
    generateBezierControlPoints(bezierPoints);
    
    // 绘制控制点和控制线
    painter.setPen(QPen(QColor(150, 200, 255), 1, Qt::DashLine));
    for (size_t i = 0; i < bezierPoints.size(); i += 3) {
        if (i + 3 < bezierPoints.size()) {
            // 绘制控制线
            painter.drawLine(bezierPoints[i], bezierPoints[i+1]);
            painter.drawLine(bezierPoints[i+2], bezierPoints[i+3]);
            
            // 绘制控制点
            painter.setBrush(QColor(255, 200, 50));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(bezierPoints[i+1], 6, 6);
            painter.drawEllipse(bezierPoints[i+2], 6, 6);
        }
    }
}

void Canvas::drawQuadraticSpline(QPainter &painter) {
    // 二次样条曲线实现
    if (controlPoints.size() < 2) return;
    
    // 使用当前曲线类型的颜色
    painter.setRenderHint(QPainter::Antialiasing, true); // 增加抗锯齿
    painter.setPen(QPen(getCurveColor(), 3));
    
    // 简单的二次样条实现
    for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
        QPointF p0 = controlPoints[i].pos;
        QPointF p1 = controlPoints[i+1].pos;
        
        // 二次贝塞尔曲线控制点（中点）
        QPointF control = (p0 + p1) / 2;
        if (i < controlPoints.size() - 2) {
            control = (p0 + controlPoints[i+2].pos) / 2;
        }
        
        // 绘制二次贝塞尔曲线
        QPointF prev = p0;
        for (double t = 0.1; t <= 1.0; t += 0.051) {
            double u = 1 - t;
            QPointF p = u*u * p0 + 2*u*t * control + t*t * p1;
            painter.drawLine(prev, p);
            prev = p;
        }
    }
}

void Canvas::drawCubicSpline(QPainter &painter) {
    // 三次样条曲线实现
    if (controlPoints.size() < 2) return;
    
    // 使用当前曲线类型的颜色
    painter.setRenderHint(QPainter::Antialiasing, true); // 增加抗锯齿
    painter.setPen(QPen(getCurveColor(), 3));
    
    // 简单的三次样条实现
    for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
        QPointF p0 = controlPoints[i].pos;
        QPointF p3 = controlPoints[i+1].pos;
        
        // 控制点（使用切线方向）
        QPointF p1 = p0 + (i > 0 ? (controlPoints[i].leftTangent) : QPointF(30, 0));
        QPointF p2 = p3 + (i < controlPoints.size()-2 ? controlPoints[i+1].rightTangent : QPointF(-30, 0));
        
        // 绘制三次贝塞尔曲线
        QPointF prev = p0;
        for (double t = 0.1; t <= 1.0; t += 0.005) {
            double u = 1 - t;
            double x = u*u*u*p0.x() + 3*u*u*t*p1.x() + 3*u*t*t*p2.x() + t*t*t*p3.x();
            double y = u*u*u*p0.y() + 3*u*u*t*p1.y() + 3*u*t*t*p2.y() + t*t*t*p3.y();
            QPointF p(x, y);
            painter.drawLine(prev, p);
            prev = p;
        }
    }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
    QPointF pos = event->pos();
    
    // 左键点击
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击了左侧切线控制点
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            QPointF tangentEnd = controlPoints[i].pos + controlPoints[i].leftTangent;
            if (distance(pos, tangentEnd) < 10) {
                controlPoints[i].selected = true;
                draggingLeftTangent = true;
                draggingPoint = i;
                
                // 取消其他点的选中状态
                for (size_t j = 0; j < controlPoints.size(); ++j) {
                    if (j != i) controlPoints[j].selected = false;
                }
                
                update();
                return;
            }
        }
        
        // 检查是否点击了右侧切线控制点
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            QPointF tangentEnd = controlPoints[i].pos + controlPoints[i].rightTangent;
            if (distance(pos, tangentEnd) < 10) {
                controlPoints[i].selected = true;
                draggingRightTangent = true;
                draggingPoint = i;
                
                // 取消其他点的选中状态
                for (size_t j = 0; j < controlPoints.size(); ++j) {
                    if (j != i) controlPoints[j].selected = false;
                }
                
                update();
                return;
            }
        }
        
        // 检查是否点击了控制点
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            if (distance(pos, controlPoints[i].pos) < 10) {
                // 选中该点
                controlPoints[i].selected = true;
                draggingPoint = i;
                draggingLeftTangent = false;
                draggingRightTangent = false;
                
                // 取消其他点的选中状态
                for (size_t j = 0; j < controlPoints.size(); ++j) {
                    if (j != i) controlPoints[j].selected = false;
                }
                
                update();
                return;
            }
        }
        
        // 添加新点
        ControlPoint newPoint(pos, nextId++);
        
        // 记录需要刷新的点索引
        std::vector<size_t> pointsToUpdate;
        
        // 自动设置合理的切线方向
        if (!controlPoints.empty()) {
            // 获取上一个点
            ControlPoint& prevPoint = controlPoints.back();
            
            // 计算从上一个点到新点的距离
            qreal dist = distance(pos, prevPoint.pos);
            
            // 设置合理的切线长度（距离的30%）
            qreal tangentLength = dist * 0.3;
            if (tangentLength < 5) tangentLength = 5; // 最小长度
            
            // 计算差值向量
            QPointF diffVector;
            if (controlPoints.size() >= 2) {
                // 有上上个点：计算上上个点到新点的差值向量
                const ControlPoint& prevPrevPoint = controlPoints[controlPoints.size() - 2];
                diffVector = pos - prevPrevPoint.pos;
                
                // 归一化差值向量
                qreal diffLength = sqrt(diffVector.x() * diffVector.x() + diffVector.y() * diffVector.y());
                if (diffLength > 0.001) {
                    diffVector = diffVector * (1.0 / diffLength);
                } else {
                    diffVector = QPointF(1, 0); // 默认方向
                }
                
                // 标记上上个点需要刷新
                pointsToUpdate.push_back(controlPoints.size() - 2);
            } else {
                // 没有上上个点（只有两个点）：使用上一个点到新点的方向
                QPointF prevToNew = pos - prevPoint.pos;
                if (dist > 0.001) {
                    diffVector = prevToNew * (1.0 / dist);
                } else {
                    diffVector = QPointF(1, 0); // 默认方向
                }
            }
            
            // 更新上一个点的左右导数
            // 左导数：指向左面（与差值向量相反方向）
            prevPoint.leftTangent = diffVector * tangentLength;
            prevPoint.leftTangentFixed = false; // 清除固定标志
            
            // 右导数：指向右面（与差值向量相同方向）
            prevPoint.rightTangent = -diffVector * tangentLength;
            prevPoint.rightTangentFixed = false; // 清除固定标志
            
            // 标记上一个点需要刷新
            pointsToUpdate.push_back(controlPoints.size() - 1);
            
            // 设置新点的左侧切线（指向上一个点）
            newPoint.leftTangent = -prevPoint.rightTangent;
        } else {
            // 第一个点的默认切线
            newPoint.leftTangent = QPointF(-20, 0);
        }
        
        // 设置新点的右侧切线（默认水平方向）
        newPoint.rightTangent = QPointF(20, 0);
        
        // 添加新点
        controlPoints.push_back(newPoint);
        draggingPoint = controlPoints.size() - 1;
        draggingLeftTangent = false;
        draggingRightTangent = false;
        
        // 强制刷新所有被影响的点
        for (size_t index : pointsToUpdate) {
            controlPoints[index].selected = true; // 临时选中以强制绘制切线
        }
        
        update();
        
        // 恢复点的选中状态
        for (size_t index : pointsToUpdate) {
            controlPoints[index].selected = false;
        }
    } 
    // 右键点击
    else if (event->button() == Qt::RightButton) {
        // 查找鼠标下的点
        int indexToDelete = -1;
        double minDist = 20;
        
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            double d = distance(pos, controlPoints[i].pos);
            if (d < minDist) {
                minDist = d;
                indexToDelete = i;
            }
        }
        
        // 如果没有点被点击，删除最近的点
        if (indexToDelete == -1 && !controlPoints.empty()) {
            indexToDelete = controlPoints.size() - 1;
        }
        
        // 删除点
        if (indexToDelete != -1) {
            // 更新相邻点的切线（如果它们没有被手动固定）
            if (indexToDelete > 0 && !controlPoints[indexToDelete-1].rightTangentFixed) {
                // 如果删除的点不是最后一个，更新前一个点的右侧切线
                if (indexToDelete < static_cast<int>(controlPoints.size()) - 1) {
                    QPointF prevToNext = controlPoints[indexToDelete+1].pos - controlPoints[indexToDelete-1].pos;
                    qreal dist = distance(controlPoints[indexToDelete-1].pos, controlPoints[indexToDelete+1].pos);
                    
                    if (dist > 0.001) {
                        prevToNext = prevToNext * (0.3 * dist / dist);
                    } else {
                        prevToNext = QPointF(20, 0);
                    }
                    
                    controlPoints[indexToDelete-1].rightTangent = prevToNext;
                }
                // 如果删除的是最后一个点，重置前一个点的右侧切线
                else {
                    controlPoints[indexToDelete-1].rightTangent = QPointF(20, 0);
                }
            }
            
            // 更新后一个点的切线（如果存在且没有被手动固定）
            if (indexToDelete < static_cast<int>(controlPoints.size()) - 1 && 
                !controlPoints[indexToDelete+1].leftTangentFixed) {
                // 如果删除的点不是第一个，更新后一个点的左侧切线
                if (indexToDelete > 0) {
                    QPointF nextToPrev = controlPoints[indexToDelete-1].pos - controlPoints[indexToDelete+1].pos;
                    qreal dist = distance(controlPoints[indexToDelete+1].pos, controlPoints[indexToDelete-1].pos);
                    
                    if (dist > 0.001) {
                        nextToPrev = nextToPrev * (0.3 * dist / dist);
                    } else {
                        nextToPrev = QPointF(-20, 0);
                    }
                    
                    controlPoints[indexToDelete+1].leftTangent = nextToPrev;
                }
                // 如果删除的是第一个点，重置后一个点的左侧切线
                else {
                    controlPoints[indexToDelete+1].leftTangent = QPointF(-20, 0);
                }
            }
            
            // 删除点
            controlPoints.erase(controlPoints.begin() + indexToDelete);
            draggingPoint = -1;
            update();
        }
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    // 更新悬停状态
    int newHoveredIndex = -1;
    double minDist = 20.0;
    
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        double d = distance(event->pos(), controlPoints[i].pos);
        if (d < minDist) {
            minDist = d;
            newHoveredIndex = i;
        }
    }
    
    if (newHoveredIndex != hoveredIndex) {
        hoveredIndex = newHoveredIndex;
        update();
        
        if (hoveredIndex >= 0) {
            emit pointHovered(controlPoints[hoveredIndex].pos);
        } else {
            emit noPointHovered();
        }
    }
    
    // 处理点拖动
    if (draggingPoint == -1) return;
    
    QPointF pos = event->pos();
    
    if (draggingLeftTangent) {
        // 拖动左侧切线
        controlPoints[draggingPoint].leftTangent = pos - controlPoints[draggingPoint].pos;
        controlPoints[draggingPoint].leftTangentFixed = true;
    } 
    else if (draggingRightTangent) {
        // 拖动右侧切线
        controlPoints[draggingPoint].rightTangent = pos - controlPoints[draggingPoint].pos;
        controlPoints[draggingPoint].rightTangentFixed = true;
    }
    else {
        // 拖动控制点
        controlPoints[draggingPoint].pos = pos;
    }
    
    update();
}

void Canvas::mouseReleaseEvent(QMouseEvent *) {
    draggingPoint = -1;
    draggingLeftTangent = false;
    draggingRightTangent = false;
}

void Canvas::leaveEvent(QEvent *event) {
    Q_UNUSED(event);
    hoveredIndex = -1;
    update();
    emit noPointHovered();
}

void Canvas::generateSpline(std::vector<QPointF>& curvePoints) {
    curvePoints.clear();
    if (controlPoints.size() < 2) return;
    
    // 生成每个段
    for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
        const auto& p0 = controlPoints[i].pos;
        const auto& p3 = controlPoints[i+1].pos;
        
        QPointF p1, p2;
        
        // 计算起点控制点
        if (controlPoints[i].rightTangentFixed) {
            p1 = p0 + controlPoints[i].rightTangent;
        } else {
            if (i == 0) {
                // 第一个点：使用到下一个点的方向
                QPointF dir = p3 - p0;
                qreal length = sqrt(dir.x() * dir.x() + dir.y() * dir.y());
                if (length > 0.001) {
                    dir = dir * (0.3 * length / length);
                } else {
                    dir = QPointF(20, 0);
                }
                p1 = p0 + dir;
            } else {
                // 使用前一个点和下一个点的平均方向
                QPointF prevDir = p0 - controlPoints[i-1].pos;
                QPointF nextDir = p3 - p0;
                
                qreal prevLength = sqrt(prevDir.x() * prevDir.x() + prevDir.y() * prevDir.y());
                qreal nextLength = sqrt(nextDir.x() * nextDir.x() + nextDir.y() * nextDir.y());
                
                // 计算加权平均方向
                QPointF avgDir = (prevDir * nextLength + nextDir * prevLength) / (prevLength + nextLength + 0.0001);
                qreal avgLength = (prevLength + nextLength) * 0.15;
                
                if (avgLength > 0.001) {
                    p1 = p0 + avgDir * (avgLength / sqrt(avgDir.x() * avgDir.x() + avgDir.y() * avgDir.y()));
                } else {
                    p1 = p0 + QPointF(20, 0);
                }
            }
        }
        
        // 计算终点控制点
        if (controlPoints[i+1].leftTangentFixed) {
            p2 = p3 + controlPoints[i+1].leftTangent;
        } else {
            if (i == controlPoints.size() - 2) {
                // 最后一个点：使用到前一个点的方向
                QPointF dir = p0 - p3;
                qreal length = sqrt(dir.x() * dir.x() + dir.y() * dir.y());
                if (length > 0.001) {
                    dir = dir * (0.3 * length / length);
                } else {
                    dir = QPointF(-20, 0);
                }
                p2 = p3 + dir;
            } else {
                // 使用当前点和下下个点的方向
                QPointF prevDir = p3 - p0;
                QPointF nextDir = controlPoints[i+2].pos - p3;
                
                qreal prevLength = sqrt(prevDir.x() * prevDir.x() + prevDir.y() * prevDir.y());
                qreal nextLength = sqrt(nextDir.x() * nextDir.x() + nextDir.y() * nextDir.y());
                
                // 计算加权平均方向
                QPointF avgDir = (prevDir * nextLength + nextDir * prevLength) / (prevLength + nextLength + 0.0001);
                qreal avgLength = (prevLength + nextLength) * 0.15;
                
                if (avgLength > 0.001) {
                    p2 = p3 + avgDir * (avgLength / sqrt(avgDir.x() * avgDir.x() + avgDir.y() * avgDir.y()));
                } else {
                    p2 = p3 + QPointF(-20, 0);
                }
            }
        }
        
        // 生成贝塞尔曲线点
        for (double t = 0; t <= 1.0; t += 0.005) {
            double u = 1 - t;
            double x = u*u*u*p0.x() + 3*u*u*t*p1.x() + 3*u*t*t*p2.x() + t*t*t*p3.x();
            double y = u*u*u*p0.y() + 3*u*u*t*p1.y() + 3*u*t*t*p2.y() + t*t*t*p3.y();
            curvePoints.push_back(QPointF(x, y));
        }
    }
}

void Canvas::generateBezierControlPoints(std::vector<QPointF>& bezierPoints) {
    bezierPoints.clear();
    if (controlPoints.size() < 2) return;
    
    for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
        const auto& p0 = controlPoints[i].pos;
        const auto& p3 = controlPoints[i+1].pos;
        
        QPointF p1, p2;
        
        // 计算起点控制点
        if (controlPoints[i].rightTangentFixed) {
            p1 = p0 + controlPoints[i].rightTangent;
        } else {
            if (i == 0) {
                QPointF dir = p3 - p0;
                qreal length = sqrt(dir.x() * dir.x() + dir.y() * dir.y());
                if (length > 0.001) {
                    dir = dir * (0.3 * length / length);
                } else {
                    dir = QPointF(20, 0);
                }
                p1 = p0 + dir;
            } else {
                QPointF prevDir = p0 - controlPoints[i-1].pos;
                QPointF nextDir = p3 - p0;
                
                qreal prevLength = sqrt(prevDir.x() * prevDir.x() + prevDir.y() * prevDir.y());
                qreal nextLength = sqrt(nextDir.x() * nextDir.x() + nextDir.y() * nextDir.y());
                
                QPointF avgDir = (prevDir * nextLength + nextDir * prevLength) / (prevLength + nextLength + 0.0001);
                qreal avgLength = (prevLength + nextLength) * 0.15;
                
                if (avgLength > 0.001) {
                    p1 = p0 + avgDir * (avgLength / sqrt(avgDir.x() * avgDir.x() + avgDir.y() * avgDir.y()));
                } else {
                    p1 = p0 + QPointF(20, 0);
                }
            }
        }
        
        // 计算终点控制点
        if (controlPoints[i+1].leftTangentFixed) {
            p2 = p3 + controlPoints[i+1].leftTangent;
        } else {
            if (i == controlPoints.size() - 2) {
                QPointF dir = p0 - p3;
                qreal length = sqrt(dir.x() * dir.x() + dir.y() * dir.y());
                if (length > 0.001) {
                    dir = dir * (0.3 * length / length);
                } else {
                    dir = QPointF(-20, 0);
                }
                p2 = p3 + dir;
            } else {
                QPointF prevDir = p3 - p0;
                QPointF nextDir = controlPoints[i+2].pos - p3;
                
                qreal prevLength = sqrt(prevDir.x() * prevDir.x() + prevDir.y() * prevDir.y());
                qreal nextLength = sqrt(nextDir.x() * nextDir.x() + nextDir.y() * nextDir.y());
                
                QPointF avgDir = (prevDir * nextLength + nextDir * prevLength) / (prevLength + nextLength + 0.0001);
                qreal avgLength = (prevLength + nextLength) * 0.15;
                
                if (avgLength > 0.001) {
                    p2 = p3 + avgDir * (avgLength / sqrt(avgDir.x() * avgDir.x() + avgDir.y() * avgDir.y()));
                } else {
                    p2 = p3 + QPointF(-20, 0);
                }
            }
        }
        
        // 添加贝塞尔控制点
        bezierPoints.push_back(p0);
        bezierPoints.push_back(p1);
        bezierPoints.push_back(p2);
        bezierPoints.push_back(p3);
    }
}

QColor Canvas::getCurveColor() const {
    // 为不同类型的曲线设置不同颜色
    switch (curveType) {
        case OriginalSpline:
            return QColor(255, 100, 100); // 红色
        case BezierCurve:
            return QColor(100, 255, 100); // 绿色
        case QuadraticSpline:
            return QColor(100, 100, 255); // 蓝色
        case CubicSpline:
            return QColor(255, 255, 100); // 黄色
        default:
            return QColor(255, 100, 100); // 默认为红色
    }
}

double Canvas::distance(const QPointF& p1, const QPointF& p2) {
    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    return sqrt(dx*dx + dy*dy);
}