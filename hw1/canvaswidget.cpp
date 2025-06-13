#include "canvaswidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

using namespace Eigen;

CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(800, 600);
    setMouseTracking(true);
}

void CanvasWidget::clearPoints()
{
    points.clear();
    hoveredIndex = -1;
    update();
    emit noPointHovered();
}

void CanvasWidget::setPolyDegree(int degree)
{
    polyDegree = degree;
    update();
}

void CanvasWidget::setGaussianSigma(double sigma)
{
    gaussianSigma = sigma;
    update();
}

void CanvasWidget::setRidgeLambda(double lambda)
{
    ridgeLambda = lambda;
    update();
}

void CanvasWidget::togglePolyInterpolation(bool enabled)
{
    showPolyInterpolation = enabled;
    update();
}

void CanvasWidget::toggleGaussianInterpolation(bool enabled)
{
    showGaussianInterpolation = enabled;
    update();
}

void CanvasWidget::toggleLeastSquares(bool enabled)
{
    showLeastSquares = enabled;
    update();
}

void CanvasWidget::toggleRidgeRegression(bool enabled)
{
    showRidgeRegression = enabled;
    update();
}

QPointF CanvasWidget::toMathCoords(const QPointF &p) const
{
    return QPointF(p.x(), height() - p.y());
}

QPointF CanvasWidget::toScreenCoords(const QPointF &p) const
{
    return QPointF(p.x(), height() - p.y());
}

int CanvasWidget::findHoveredPoint(const QPointF &pos) const
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

void CanvasWidget::deletePoint(int index)
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

void CanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.fillRect(rect(), Qt::white);
    
    // 绘制网格
    drawGrid(painter);
    
    // 绘制曲线
    drawCurves(painter);
    
    // 绘制点
    drawPoints(painter);
    
    // 绘制悬停指示器
    if (hoveredIndex >= 0) {
        drawHoverIndicator(painter);
    }
}

void CanvasWidget::drawGrid(QPainter &painter)
{
    painter.setPen(QPen(QColor(240, 240, 240), 1));
    
    // 绘制水平网格线
    for (int y = 0; y < height(); y += 20) {
        painter.drawLine(0, y, width(), y);
    }
    
    // 绘制垂直网格线
    for (int x = 0; x < width(); x += 20) {
        painter.drawLine(x, 0, x, height());
    }
}

void CanvasWidget::drawPoints(QPainter &painter)
{
    painter.setPen(Qt::black);
    
    for (int i = 0; i < points.size(); ++i) {
        if (i == hoveredIndex) {
            painter.setBrush(QColor(255, 100, 100)); // 悬停状态的点
        } else {
            painter.setBrush(Qt::red); // 普通点
        }
        painter.drawEllipse(points[i].pos, 6, 6);
    }
}

void CanvasWidget::drawHoverIndicator(QPainter &painter)
{
    if (hoveredIndex < 0 || hoveredIndex >= points.size()) 
        return;
        
    const QPointF &p = points[hoveredIndex].pos;
    
    // 绘制坐标文本背景
    QRectF textRect(p.x() + 15, p.y() - 30, 120, 25);
    painter.setBrush(QColor(255, 255, 220, 220));
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.drawRoundedRect(textRect, 5, 5);
    
    // 绘制坐标文本
    QPointF mathPoint = toMathCoords(p);
    QString coordText = QString("(%1, %2)").arg(mathPoint.x(), 0, 'f', 1).arg(mathPoint.y(), 0, 'f', 1);
    
    painter.setPen(Qt::black);
    painter.drawText(textRect, Qt::AlignCenter, coordText);
    
    // 绘制连接线
    painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    painter.drawLine(p, QPointF(p.x() + 15, p.y() - 15));
}

void CanvasWidget::drawCurves(QPainter &painter)
{
    if (points.size() < 2) return;
    
    if (showPolyInterpolation) {
        QVector<QPointF> curve = calculatePolynomialInterpolation();
        painter.setPen(QPen(Qt::blue, 2));
        for (int i = 1; i < curve.size(); ++i) {
            painter.drawLine(curve[i-1], curve[i]);
        }
    }
    
    if (showGaussianInterpolation) {
        QVector<QPointF> curve = calculateGaussianInterpolation();
        painter.setPen(QPen(Qt::darkGreen, 2));
        for (int i = 1; i < curve.size(); ++i) {
            painter.drawLine(curve[i-1], curve[i]);
        }
    }
    
    if (showLeastSquares) {
        QVector<QPointF> curve = calculateLeastSquares();
        painter.setPen(QPen(Qt::magenta, 2));
        for (int i = 1; i < curve.size(); ++i) {
            painter.drawLine(curve[i-1], curve[i]);
        }
    }
    
    if (showRidgeRegression) {
        QVector<QPointF> curve = calculateRidgeRegression();
        painter.setPen(QPen(Qt::darkCyan, 2));
        for (int i = 1; i < curve.size(); ++i) {
            painter.drawLine(curve[i-1], curve[i]);
        }
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent *event)
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

void CanvasWidget::mouseMoveEvent(QMouseEvent *event)
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

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event)
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

void CanvasWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // 直接删除点，无需菜单
    int pointIndex = findHoveredPoint(event->pos());
    
    if (pointIndex >= 0) {
        // 删除悬停的点
        deletePoint(pointIndex);
    } else if (!points.isEmpty()) {
        // 删除最后一个点
        deletePoint(points.size() - 1);
    }
}

void CanvasWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredIndex = -1;
    update();
    emit noPointHovered();
}

// QVector<QPointF> CanvasWidget::calculatePolynomialInterpolation()
// {
//     int n = points.size();
//     VectorXd x(n), y(n);
    
//     // 填充数据
//     for (int i = 0; i < n; ++i) {
//         QPointF mathPoint = toMathCoords(points[i].pos);
//         x(i) = mathPoint.x();
//         y(i) = mathPoint.y();
//     }
    
//     // 创建范德蒙矩阵
//     MatrixXd A(n, n);
//     for (int i = 0; i < n; ++i) {
//         for (int j = 0; j < n; ++j) {
//             A(i, j) = pow(x(i), j);
//         }
//     }
    
//     // 解线性方程组
//     VectorXd coeffs = A.colPivHouseholderQr().solve(y);
    
//     // 生成曲线
//     QVector<QPointF> curve;
//     for (int px = 0; px < width(); px += 2) {
//         double py = 0;
//         for (int j = 0; j < n; ++j) {
//             py += coeffs(j) * pow(px, j);
//         }
//         curve.append(toScreenCoords(QPointF(px, py)));
//     }
//     return curve;
// }

QVector<QPointF> CanvasWidget::calculatePolynomialInterpolation()
{
    int n = points.size();
    if (n == 0) return QVector<QPointF>();
    
    // 转换并排序点（牛顿插值需要有序节点）
    QVector<QPointF> mathPoints;
    for (int i = 0; i < n; ++i) {
        mathPoints.append(toMathCoords(points[i].pos));
    }
    std::sort(mathPoints.begin(), mathPoints.end(), 
              [](const QPointF& a, const QPointF& b) { 
                  return a.x() < b.x(); 
              });
    
    // 提取坐标
    VectorXd x(n), y(n);
    for (int i = 0; i < n; ++i) {
        x(i) = mathPoints[i].x();
        y(i) = mathPoints[i].y();
    }
    
    // 计算差商表 (牛顿插值核心)
    MatrixXd F(n, n);
    for (int i = 0; i < n; ++i) {
        F(i, 0) = y(i);  // 0阶差商
    }
    for (int j = 1; j < n; ++j) {
        for (int i = j; i < n; ++i) {
            F(i, j) = (F(i, j-1) - F(i-1, j-1)) / (x(i) - x(i-j));
        }
    }
    
    // 生成曲线
    QVector<QPointF> curve;
    for (int px = 0; px < width(); px += 2) {
        double mathX = toMathCoords(QPointF(px, 0)).x();  // 转换为数学坐标系
        
        // 牛顿插值公式
        double mathY = F(0, 0);  // 初始值 = f(x0)
        double product = 1.0;
        for (int j = 1; j < n; ++j) {
            product *= (mathX - x(j-1));  // 累乘(x-x0)(x-x1)...
            mathY += F(j, j) * product;   // 添加差商项
        }
        
        curve.append(toScreenCoords(QPointF(mathX, mathY)));
    }
    return curve;
}

QVector<QPointF> CanvasWidget::calculateGaussianInterpolation()
{
    int n = points.size();
    if (n < 1) return QVector<QPointF>();
    
    VectorXd x(n), y(n);
    
    // 填充数据（转换为数学坐标系）
    for (int i = 0; i < n; ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // 创建高斯核矩阵
    MatrixXd A(n, n);
    double sigma_sq = gaussianSigma * gaussianSigma;
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double dist = x(i) - x(j);
            // 高斯核函数：K(x_i, x_j) = exp(-||x_i - x_j||^2 / (2 * sigma^2))
            A(i, j) = exp(-dist * dist / (2 * sigma_sq));
        }
    }
    
    // 添加正则化项防止矩阵奇异
    MatrixXd I = MatrixXd::Identity(n, n);
    MatrixXd A_reg = A + 1e-6 * I;
    
    // 解线性方程组
    VectorXd weights = A_reg.colPivHouseholderQr().solve(y);
    
    // 生成曲线
    QVector<QPointF> curve;
    for (int px = 0; px < width(); px += 2) {
        double math_x = px; // 屏幕x坐标等于数学x坐标
        double math_y = 0;
        
        for (int j = 0; j < n; ++j) {
            double dist = math_x - x(j);
            math_y += weights(j) * exp(-dist * dist / (2 * sigma_sq));
        }
        
        curve.append(toScreenCoords(QPointF(math_x, math_y)));
    }
    return curve;
}

QVector<QPointF> CanvasWidget::calculateLeastSquares()
{
    int n = points.size();
    if (n <= polyDegree) return QVector<QPointF>();
    
    VectorXd x(n), y(n);
    
    // 填充数据
    for (int i = 0; i < n; ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // 创建设计矩阵
    MatrixXd A(n, polyDegree + 1);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= polyDegree; ++j) {
            A(i, j) = pow(x(i), j);
        }
    }
    
    // 解最小二乘问题
    VectorXd coeffs = (A.transpose() * A).ldlt().solve(A.transpose() * y);
    
    // 生成曲线
    QVector<QPointF> curve;
    for (int px = 0; px < width(); px += 2) {
        double py = 0;
        for (int j = 0; j <= polyDegree; ++j) {
            py += coeffs(j) * pow(px, j);
        }
        curve.append(toScreenCoords(QPointF(px, py)));
    }
    return curve;
}

QVector<QPointF> CanvasWidget::calculateRidgeRegression()
{
    int n = points.size();
    if (n <= polyDegree) return QVector<QPointF>();
    
    VectorXd x(n), y(n);
    
    // 填充数据
    for (int i = 0; i < n; ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // 创建设计矩阵
    MatrixXd A(n, polyDegree + 1);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= polyDegree; ++j) {
            A(i, j) = pow(x(i), j);
        }
    }
    
    // 创建正则化矩阵
    MatrixXd I = MatrixXd::Identity(polyDegree + 1, polyDegree + 1);
    
    // 解岭回归问题
    VectorXd coeffs = (A.transpose() * A + ridgeLambda * I).ldlt().solve(A.transpose() * y);
    
    // 生成曲线
    QVector<QPointF> curve;
    for (int px = 0; px < width(); px += 2) {
        double py = 0;
        for (int j = 0; j <= polyDegree; ++j) {
            py += coeffs(j) * pow(px, j);
        }
        curve.append(toScreenCoords(QPointF(px, py)));
    }
    return curve;
}