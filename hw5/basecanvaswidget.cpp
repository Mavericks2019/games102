#include "basecanvaswidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>

BaseCanvasWidget::BaseCanvasWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(800, 600);
    setMouseTracking(true);
}

QPointF BaseCanvasWidget::toMathCoords(const QPointF &p) const
{
    return QPointF(p.x(), height() - p.y());
}

QPointF BaseCanvasWidget::toScreenCoords(const QPointF &p) const
{
    return QPointF(p.x(), height() - p.y());
}

void BaseCanvasWidget::clearPoints()
{
    points.clear();
    hoveredIndex = -1;
    update();
    emit noPointHovered();
}

void BaseCanvasWidget::setCurveColor(const QColor &color)
{
    curveColor = color;
    update();
}

void BaseCanvasWidget::drawGrid(QPainter &painter)
{
    painter.setPen(QPen(QColor(240, 240, 240), 1));
    
    // 水平网格线
    for (int y = 0; y < height(); y += 20) {
        painter.drawLine(0, y, width(), y);
    }
    
    // 垂直网格线
    for (int x = 0; x < width(); x += 20) {
        painter.drawLine(x, 0, x, height());
    }
}

void BaseCanvasWidget::drawPoints(QPainter &painter)
{
    painter.setPen(Qt::black);
    
    for (int i = 0; i < points.size(); ++i) {
        if (i == hoveredIndex) {
            painter.setBrush(QColor(255, 100, 100)); // 悬停点
        } else {
            painter.setBrush(Qt::red); // 普通点
        }
        painter.drawEllipse(points[i].pos, 6, 6);
    }
}

void BaseCanvasWidget::drawHoverIndicator(QPainter &painter)
{
    if (hoveredIndex < 0 || hoveredIndex >= points.size()) 
        return;
        
    const QPointF &p = points[hoveredIndex].pos;
    
    // 坐标文本背景
    QRectF textRect(p.x() + 15, p.y() - 30, 120, 25);
    painter.setBrush(QColor(255, 255, 220, 220));
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.drawRoundedRect(textRect, 5, 5);
    
    // 坐标文本
    QPointF mathPoint = toMathCoords(p);
    QString coordText = QString("(%1, %2)").arg(mathPoint.x(), 0, 'f', 1).arg(mathPoint.y(), 0, 'f', 1);
    
    painter.setPen(Qt::black);
    painter.drawText(textRect, Qt::AlignCenter, coordText);
    
    // 连接线
    painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    painter.drawLine(p, QPointF(p.x() + 15, p.y() - 15));
}

void BaseCanvasWidget::drawInfoPanel(QPainter &painter)
{
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, 20, "Base Canvas");
}

void BaseCanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.fillRect(rect(), Qt::white);
    
    // 绘制网格
    drawGrid(painter);
    
    // 绘制曲线
    if (showCurve) {
        drawCurves(painter);
    }
    
    // 绘制点
    drawPoints(painter);
    
    // 绘制信息面板
    drawInfoPanel(painter);
    
    // 绘制悬停指示器
    if (hoveredIndex >= 0) {
        drawHoverIndicator(painter);
    }
}

void BaseCanvasWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        selectedIndex = -1;
        hoveredIndex = findHoveredPoint(event->pos());
        
        if (hoveredIndex >= 0) {
            selectedIndex = hoveredIndex;
            points[selectedIndex].moving = true;
            emit pointHovered(points[selectedIndex].pos);
        }
    }
}

void BaseCanvasWidget::mouseMoveEvent(QMouseEvent *event)
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
    if ((event->buttons() & Qt::LeftButton) && selectedIndex >= 0) {
        points[selectedIndex].pos = event->pos();
        emit pointHovered(points[selectedIndex].pos);
        update();
    }
}

void BaseCanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selectedIndex >= 0) {
            points[selectedIndex].moving = false;
            selectedIndex = -1;
        } else {
            // 添加新点
            points.append({event->pos(), false});
            hoveredIndex = points.size() - 1;
            emit pointHovered(points.last().pos);
        }
        update();
    }
}

void BaseCanvasWidget::contextMenuEvent(QContextMenuEvent *event)
{
    int pointIndex = findHoveredPoint(event->pos());
    
    if (pointIndex >= 0) {
        deletePoint(pointIndex);
    } else if (!points.isEmpty()) {
        deletePoint(points.size() - 1);
    }
}

void BaseCanvasWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredIndex = -1;
    update();
    emit noPointHovered();
}

int BaseCanvasWidget::findHoveredPoint(const QPointF &pos) const
{
    const int hoverRadius = 15;
    
    for (int i = 0; i < points.size(); ++i) {
        QPointF diff = pos - points[i].pos;
        if (diff.manhattanLength() < hoverRadius) {
            return i;
        }
    }
    return -1;
}

void BaseCanvasWidget::deletePoint(int index)
{
    if (index < 0 || index >= points.size()) 
        return;
        
    points.remove(index);
    
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