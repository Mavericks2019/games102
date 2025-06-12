#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <Eigen/Dense>

class CanvasWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit CanvasWidget(QWidget *parent = nullptr);
    
    void clearPoints();
    void setPolyDegree(int degree);
    void setGaussianSigma(double sigma);
    void setRidgeLambda(double lambda);
    
    void togglePolyInterpolation(bool enabled);
    void toggleGaussianInterpolation(bool enabled);
    void toggleLeastSquares(bool enabled);
    void toggleRidgeRegression(bool enabled);
    
    QSize sizeHint() const override { return QSize(800, 600); }
    
signals:
    void pointHovered(const QPointF& point);
    void noPointHovered();
    void pointDeleted();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    
private:
    struct Point {
        QPointF pos;
        bool moving = false;
    };
    
    QVector<Point> points;
    int selectedIndex = -1;
    int hoveredIndex = -1;
    
    bool showPolyInterpolation = false;
    bool showGaussianInterpolation = false;
    bool showLeastSquares = false;
    bool showRidgeRegression = false;
    
    int polyDegree = 3;
    double gaussianSigma = 10.0;
    double ridgeLambda = 0.1;
    
    void drawGrid(QPainter &painter);
    void drawPoints(QPainter &painter);
    void drawCurves(QPainter &painter);
    void drawHoverIndicator(QPainter &painter);
    
    QVector<QPointF> calculatePolynomialInterpolation();
    QVector<QPointF> calculateGaussianInterpolation();
    QVector<QPointF> calculateLeastSquares();
    QVector<QPointF> calculateRidgeRegression();
    
    QPointF toMathCoords(const QPointF &p) const;
    QPointF toScreenCoords(const QPointF &p) const;
    
    int findHoveredPoint(const QPointF &pos) const;
    void deletePoint(int index);
};

#endif // CANVASWIDGET_H