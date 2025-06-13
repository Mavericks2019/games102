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
    tValues.clear();
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

void CanvasWidget::setParameterizationMethod(ParameterizationMethod method)
{
    paramMethod = method;
    calculateParameterization();
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
    
    // Update hover index
    if (hoveredIndex == index) {
        hoveredIndex = -1;
        emit noPointHovered();
    } else if (hoveredIndex > index) {
        hoveredIndex--;
    }
    
    // Update selected index
    if (selectedIndex == index) {
        selectedIndex = -1;
    } else if (selectedIndex > index) {
        selectedIndex--;
    }
    
    calculateParameterization();
    update();
    emit pointDeleted();
}

void CanvasWidget::calculateParameterization()
{
    tValues.clear();
    if (points.size() < 2) return;
    
    // Uniform parameterization
    if (paramMethod == UNIFORM) {
        for (int i = 0; i < points.size(); ++i) {
            tValues.append(static_cast<double>(i) / (points.size() - 1));
        }
    }
    // Chordal parameterization
    else if (paramMethod == CHORDAL) {
        tValues.append(0.0);
        double totalLength = 0.0;
        QVector<double> segmentLengths;
        
        // Calculate segment lengths
        for (int i = 1; i < points.size(); ++i) {
            QPointF p1 = toMathCoords(points[i-1].pos);
            QPointF p2 = toMathCoords(points[i].pos);
            double dx = p2.x() - p1.x();
            double dy = p2.y() - p1.y();
            double length = std::sqrt(dx*dx + dy*dy);
            segmentLengths.append(length);
            totalLength += length;
        }
        
        // Calculate cumulative parameters
        for (int i = 0; i < segmentLengths.size(); ++i) {
            double t = tValues.last() + (segmentLengths[i] / totalLength);
            tValues.append(t);
        }
    }
    // Centripetal parameterization
    else if (paramMethod == CENTRIPETAL) {
        tValues.append(0.0);
        double totalLength = 0.0;
        QVector<double> segmentLengths;
        
        // Calculate segment lengths
        for (int i = 1; i < points.size(); ++i) {
            QPointF p1 = toMathCoords(points[i-1].pos);
            QPointF p2 = toMathCoords(points[i].pos);
            double dx = p2.x() - p1.x();
            double dy = p2.y() - p1.y();
            double length = std::sqrt(std::sqrt(dx*dx + dy*dy));
            segmentLengths.append(length);
            totalLength += length;
        }
        
        // Calculate cumulative parameters
        for (int i = 0; i < segmentLengths.size(); ++i) {
            double t = tValues.last() + (segmentLengths[i] / totalLength);
            tValues.append(t);
        }
    }
    // Foley-Nielsen parameterization
    else if (paramMethod == FOLEY) {
        tValues.append(0.0);
        double totalLength = 0.0;
        QVector<double> segmentLengths;
        
        // Calculate segment lengths with angle weighting
        for (int i = 1; i < points.size(); ++i) {
            QPointF p0 = toMathCoords(points[i-1].pos);
            QPointF p1 = toMathCoords(points[i].pos);
            
            double dx = p1.x() - p0.x();
            double dy = p1.y() - p0.y();
            double length = std::sqrt(dx*dx + dy*dy);
            
            // Angle weighting
            double angleWeight = 1.0;
            if (i < points.size() - 1) {
                QPointF p2 = toMathCoords(points[i+1].pos);
                
                double dx1 = p1.x() - p0.x();
                double dy1 = p1.y() - p0.y();
                double dx2 = p2.x() - p1.x();
                double dy2 = p2.y() - p1.y();
                
                double dot = dx1*dx2 + dy1*dy2;
                double mag1 = std::sqrt(dx1*dx1 + dy1*dy1);
                double mag2 = std::sqrt(dx2*dx2 + dy2*dy2);
                
                if (mag1 > 0.001 && mag2 > 0.001) {
                    double cosTheta = dot / (mag1 * mag2);
                    cosTheta = std::max(-1.0, std::min(1.0, cosTheta));
                    double theta = std::acos(cosTheta);
                    angleWeight = 1.0 + 1.5 * theta * (mag1 + mag2) / (2.0 * std::min(mag1, mag2));
                }
            }
            
            segmentLengths.append(length * angleWeight);
            totalLength += length * angleWeight;
        }
        
        // Calculate cumulative parameters
        for (int i = 0; i < segmentLengths.size(); ++i) {
            double t = tValues.last() + (segmentLengths[i] / totalLength);
            tValues.append(t);
        }
    }
}

void CanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    painter.fillRect(rect(), Qt::white);
    
    // Draw grid
    drawGrid(painter);
    
    // Draw curves
    drawCurves(painter);
    
    // Draw points
    drawPoints(painter);
    
    // Draw parameterization info
    drawParameterizationInfo(painter);
    
    // Draw hover indicator
    if (hoveredIndex >= 0) {
        drawHoverIndicator(painter);
    }
}

void CanvasWidget::drawGrid(QPainter &painter)
{
    painter.setPen(QPen(QColor(240, 240, 240), 1));
    
    // Draw horizontal grid lines
    for (int y = 0; y < height(); y += 20) {
        painter.drawLine(0, y, width(), y);
    }
    
    // Draw vertical grid lines
    for (int x = 0; x < width(); x += 20) {
        painter.drawLine(x, 0, x, height());
    }
}

void CanvasWidget::drawPoints(QPainter &painter)
{
    painter.setPen(Qt::black);
    
    for (int i = 0; i < points.size(); ++i) {
        if (i == hoveredIndex) {
            painter.setBrush(QColor(255, 100, 100)); // Hovered point
        } else {
            painter.setBrush(Qt::red); // Normal point
        }
        painter.drawEllipse(points[i].pos, 6, 6);
    }
}

void CanvasWidget::drawHoverIndicator(QPainter &painter)
{
    if (hoveredIndex < 0 || hoveredIndex >= points.size()) 
        return;
        
    const QPointF &p = points[hoveredIndex].pos;
    
    // Draw coordinate text background
    QRectF textRect(p.x() + 15, p.y() - 30, 120, 25);
    painter.setBrush(QColor(255, 255, 220, 220));
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.drawRoundedRect(textRect, 5, 5);
    
    // Draw coordinate text
    QPointF mathPoint = toMathCoords(p);
    QString coordText = QString("(%1, %2)").arg(mathPoint.x(), 0, 'f', 1).arg(mathPoint.y(), 0, 'f', 1);
    
    painter.setPen(Qt::black);
    painter.drawText(textRect, Qt::AlignCenter, coordText);
    
    // Draw connection line
    painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    painter.drawLine(p, QPointF(p.x() + 15, p.y() - 15));
}

void CanvasWidget::drawParameterizationInfo(QPainter &painter)
{
    if (points.size() < 2) return;
    
    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 9));
    
    QString methodName;
    switch (paramMethod) {
        case UNIFORM: methodName = "Uniform"; break;
        case CHORDAL: methodName = "Chordal"; break;
        case CENTRIPETAL: methodName = "Centripetal"; break;
        case FOLEY: methodName = "Foley-Nielsen"; break;
    }
    
    QString info = QString("Parameterization: %1").arg(methodName);
    painter.drawText(10, 20, info);
    
    // Draw t-values near points
    painter.setPen(Qt::darkBlue);
    for (int i = 0; i < points.size(); ++i) {
        if (i < tValues.size()) {
            QString tText = QString("t=%1").arg(tValues[i], 0, 'f', 2);
            painter.drawText(points[i].pos.x() + 10, points[i].pos.y() - 15, tText);
        }
    }
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
    // Update hover state
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
    
    // Handle point dragging
    if ((event->buttons() & Qt::LeftButton) && selectedIndex >= 0) {
        points[selectedIndex].pos = event->pos();
        emit pointHovered(points[selectedIndex].pos);
        calculateParameterization();
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
            // Add new point
            points.append({event->pos(), false});
            hoveredIndex = points.size() - 1;
            emit pointHovered(points.last().pos);
            calculateParameterization();
        }
        update();
    }
}

void CanvasWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // Directly delete points without menu
    int pointIndex = findHoveredPoint(event->pos());
    
    if (pointIndex >= 0) {
        // Delete hovered point
        deletePoint(pointIndex);
    } else if (!points.isEmpty()) {
        // Delete last point
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
//     if (n < 2) return QVector<QPointF>();
    
//     VectorXd t(n), x(n), y(n);
    
//     // Fill data (convert to mathematical coordinates)
//     for (int i = 0; i < n; ++i) {
//         QPointF mathPoint = toMathCoords(points[i].pos);
//         t(i) = tValues[i];
//         x(i) = mathPoint.x();
//         y(i) = mathPoint.y();
//     }
    
//     // Create Vandermonde matrix
//     MatrixXd A(n, n);
//     for (int i = 0; i < n; ++i) {
//         for (int j = 0; j < n; ++j) {
//             A(i, j) = pow(t(i), j);
//         }
//     }
    
//     // Solve linear systems
//     VectorXd xCoeffs = A.colPivHouseholderQr().solve(x);
//     VectorXd yCoeffs = A.colPivHouseholderQr().solve(y);
    
//     // Generate curve
//     QVector<QPointF> curve;
//     for (double tVal = 0.0; tVal <= 1.0; tVal += 0.005) {
//         double px = 0;
//         double py = 0;
//         for (int j = 0; j < n; ++j) {
//             px += xCoeffs(j) * pow(tVal, j);
//             py += yCoeffs(j) * pow(tVal, j);
//         }
//         curve.append(toScreenCoords(QPointF(px, py)));
//     }
//     return curve;
// }

QVector<QPointF> CanvasWidget::calculatePolynomialInterpolation()
{
    int n = points.size();
    if (n < 2) return QVector<QPointF>();
    
    // 准备数据容器
    QVector<double> t(n);
    QVector<QPointF> mathPoints(n);
    
    // 填充数据（转换为数学坐标系）
    for (int i = 0; i < n; ++i) {
        mathPoints[i] = toMathCoords(points[i].pos);
        t[i] = tValues[i];
    }
    
    // 根据t值排序（牛顿插值需要有序节点）
    QVector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return t[a] < t[b];
    });
    
    // 提取排序后的数据
    VectorXd t_sorted(n), x_sorted(n), y_sorted(n);
    for (int i = 0; i < n; ++i) {
        int idx = indices[i];
        t_sorted(i) = t[idx];
        x_sorted(i) = mathPoints[idx].x();
        y_sorted(i) = mathPoints[idx].y();
    }
    
    // 计算差商表（牛顿插值核心）
    MatrixXd Fx(n, n), Fy(n, n);
    
    // 初始化0阶差商
    for (int i = 0; i < n; ++i) {
        Fx(i, 0) = x_sorted(i);
        Fy(i, 0) = y_sorted(i);
    }
    
    // 计算高阶差商
    for (int j = 1; j < n; ++j) {
        for (int i = j; i < n; ++i) {
            Fx(i, j) = (Fx(i, j-1) - Fx(i-1, j-1)) / (t_sorted(i) - t_sorted(i-j));
            Fy(i, j) = (Fy(i, j-1) - Fy(i-1, j-1)) / (t_sorted(i) - t_sorted(i-j));
        }
    }
    
    // 提取对角线元素（牛顿插值系数）
    VectorXd cx(n), cy(n);
    for (int i = 0; i < n; ++i) {
        cx(i) = Fx(i, i);
        cy(i) = Fy(i, i);
    }
    
    // 生成曲线
    QVector<QPointF> curve;
    for (double tVal = 0.0; tVal <= 1.0; tVal += 0.005) {
        double xVal = cx(n-1);
        double yVal = cy(n-1);
        
        // 牛顿插值公式（反向嵌套乘法）
        for (int j = n-2; j >= 0; j--) {
            xVal = xVal * (tVal - t_sorted(j)) + cx(j);
            yVal = yVal * (tVal - t_sorted(j)) + cy(j);
        }
        
        curve.append(toScreenCoords(QPointF(xVal, yVal)));
    }
    return curve;
}

QVector<QPointF> CanvasWidget::calculateGaussianInterpolation()
{
    int n = points.size();
    if (n < 1) return QVector<QPointF>();
    
    VectorXd t(n), x(n), y(n);
    
    // Fill data (convert to mathematical coordinates)
    for (int i = 0; i < n; ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        t(i) = tValues[i];
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // Create Gaussian kernel matrix
    MatrixXd A(n, n);
    double sigma_sq = gaussianSigma * gaussianSigma;
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double dist = t(i) - t(j);
            A(i, j) = exp(-dist * dist / (2 * sigma_sq));
        }
    }
    
    // Add regularization to prevent singularity
    MatrixXd I = MatrixXd::Identity(n, n);
    MatrixXd A_reg = A + 1e-6 * I;
    
    // Solve linear systems
    VectorXd xWeights = A_reg.colPivHouseholderQr().solve(x);
    VectorXd yWeights = A_reg.colPivHouseholderQr().solve(y);
    
    // Generate curve
    QVector<QPointF> curve;
    for (double tVal = 0.0; tVal <= 1.0; tVal += 0.005) {
        double px = 0;
        double py = 0;
        for (int j = 0; j < n; ++j) {
            double dist = tVal - t(j);
            px += xWeights(j) * exp(-dist * dist / (2 * sigma_sq));
            py += yWeights(j) * exp(-dist * dist / (2 * sigma_sq));
        }
        curve.append(toScreenCoords(QPointF(px, py)));
    }
    return curve;
}


QVector<QPointF> CanvasWidget::calculateLeastSquares()
{
    int n = points.size();
    if (n <= polyDegree) return QVector<QPointF>();
    
    VectorXd t(n), x(n), y(n);
    
    // Fill data
    for (int i = 0; i < n; ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        t(i) = tValues[i];
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // Create design matrix
    MatrixXd A(n, polyDegree + 1);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= polyDegree; ++j) {
            A(i, j) = pow(t(i), j);
        }
    }
    
    // Solve least squares problems
    VectorXd xCoeffs = (A.transpose() * A).ldlt().solve(A.transpose() * x);
    VectorXd yCoeffs = (A.transpose() * A).ldlt().solve(A.transpose() * y);
    
    // Generate curve
    QVector<QPointF> curve;
    for (double tVal = 0.0; tVal <= 1.0; tVal += 0.005) {
        double px = 0;
        double py = 0;
        for (int j = 0; j <= polyDegree; ++j) {
            px += xCoeffs(j) * pow(tVal, j);
            py += yCoeffs(j) * pow(tVal, j);
        }
        curve.append(toScreenCoords(QPointF(px, py)));
    }
    return curve;
}

QVector<QPointF> CanvasWidget::calculateRidgeRegression()
{
    int n = points.size();
    if (n <= polyDegree) return QVector<QPointF>();
    
    VectorXd t(n), x(n), y(n);
    
    // Fill data
    for (int i = 0; i < n; ++i) {
        QPointF mathPoint = toMathCoords(points[i].pos);
        t(i) = tValues[i];
        x(i) = mathPoint.x();
        y(i) = mathPoint.y();
    }
    
    // Create design matrix
    MatrixXd A(n, polyDegree + 1);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= polyDegree; ++j) {
            A(i, j) = pow(t(i), j);
        }
    }
    
    // Create regularization matrix
    MatrixXd I = MatrixXd::Identity(polyDegree + 1, polyDegree + 1);
    
    // Solve ridge regression problems
    VectorXd xCoeffs = (A.transpose() * A + ridgeLambda * I).ldlt().solve(A.transpose() * x);
    VectorXd yCoeffs = (A.transpose() * A + ridgeLambda * I).ldlt().solve(A.transpose() * y);
    
    // Generate curve
    QVector<QPointF> curve;
    for (double tVal = 0.0; tVal <= 1.0; tVal += 0.005) {
        double px = 0;
        double py = 0;
        for (int j = 0; j <= polyDegree; ++j) {
            px += xCoeffs(j) * pow(tVal, j);
            py += yCoeffs(j) * pow(tVal, j);
        }
        curve.append(toScreenCoords(QPointF(px, py)));
    }
    return curve;
}