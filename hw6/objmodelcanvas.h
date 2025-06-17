#ifndef OBJMODELCANVAS_H
#define OBJMODELCANVAS_H

#include "basecanvaswidget.h"
#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>

class ObjModelCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit ObjModelCanvas(QWidget *parent = nullptr);
    
    bool loadObjFile(const QString &filePath);
    void resetView();
    
    // 重写基类方法
    void drawGrid(QPainter &painter) override {} // 不绘制网格
    void drawCurves(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    struct Face {
        QVector<int> vertexIndices;
    };
    
    void calculateBoundingBox();
    void centerAndScaleModel();
    QPointF projectToScreen(const QVector3D &point) const;
    
    QVector<QVector3D> vertices;
    QVector<Face> faces;
    QVector3D minBounds;
    QVector3D maxBounds;
    QVector3D modelCenter;
    float scaleFactor = 1.0f;
    
    QMatrix4x4 rotationMatrix;
    QPoint lastMousePos;
    bool modelLoaded = false;
    QString currentModelName;
};

#endif // OBJMODELCANVAS_H