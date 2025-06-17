#ifndef BASECANVASWIDGET_H
#define BASECANVASWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPointF>
#include <QVector>
#include <QColor>
#include <math.h>

class BaseCanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseCanvasWidget(QWidget *parent = nullptr);
    
    // 坐标转换
    QPointF toMathCoords(const QPointF &p) const;
    QPointF toScreenCoords(const QPointF &p) const;
    
    // 清空画布
    virtual void clearPoints();
    
    // 绘制方法
    virtual void drawGrid(QPainter &painter);
    virtual void drawPoints(QPainter &painter);
    virtual void drawCurves(QPainter &painter) = 0;
    virtual void drawHoverIndicator(QPainter &painter);
    virtual void drawInfoPanel(QPainter &painter);
    
    // 设置曲线颜色
    void setCurveColor(const QColor &color);
    
    // 事件处理
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    
    // 查找悬停点
    virtual int findHoveredPoint(const QPointF &pos) const;
    
    // 删除点
    virtual void deletePoint(int index);
    double distance(const QPointF &p1, const QPointF &p2)
    {
        double dx = p1.x() - p2.x();
        double dy = p1.y() - p2.y();
        return std::sqrt(dx*dx + dy*dy);
    }

signals:
    void pointHovered(const QPointF& point);
    void noPointHovered();
    void pointDeleted();

protected:
    struct Point {
        QPointF pos;
        bool moving = false;
        bool movable = true;
    };
    QVector<Point> points;
    int selectedIndex = -1;
    int hoveredIndex = -1;
    QColor curveColor = Qt::blue;
    bool showCurve = true;
};

#endif // BASECANVASWIDGET_H