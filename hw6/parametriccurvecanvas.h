#ifndef PARAMETRICCURVECANVAS_H
#define PARAMETRICCURVECANVAS_H

#include "basecanvaswidget.h"
#include <QVector>
#include <Eigen/Dense>

class ParametricCurveCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit ParametricCurveCanvas(QWidget *parent = nullptr);
    
    // 参数化方法
    enum ParameterizationMethod {
        UNIFORM,
        CHORDAL,
        CENTRIPETAL,
        FOLEY
    };
    
    void setPolyDegree(int degree);
    void setGaussianSigma(double sigma);
    void setRidgeLambda(double lambda);
    void setParameterizationMethod(ParameterizationMethod method);
    
    void togglePolyInterpolation(bool enabled);
    void toggleGaussianInterpolation(bool enabled);
    void toggleLeastSquares(bool enabled);
    void toggleRidgeRegression(bool enabled);
    void toggleCurveVisibility(bool visible) { showCurve = visible; update(); }
    
    // 重写基类方法
    void clearPoints() override;
    void drawCurves(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;

private:
    void calculateParameterization();
    
    QVector<QPointF> calculatePolynomialInterpolation();
    QVector<QPointF> calculateGaussianInterpolation();
    QVector<QPointF> calculateLeastSquares();
    QVector<QPointF> calculateRidgeRegression();
    
    QVector<double> tValues; // 每个点的参数值
    ParameterizationMethod paramMethod = UNIFORM;
    int polyDegree = 3;
    double gaussianSigma = 10.0;
    double ridgeLambda = 0.1;
    
    bool showPolyInterpolation = false;
    bool showGaussianInterpolation = false;
    bool showLeastSquares = false;
    bool showRidgeRegression = false;
};

#endif // PARAMETRICCURVECANVAS_H