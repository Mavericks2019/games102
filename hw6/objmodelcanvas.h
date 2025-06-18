#ifndef OBJMODELCANVAS_H
#define OBJMODELCANVAS_H

#include "basecanvaswidget.h"
#include <Eigen/Dense>
#include <vector>
#include <QVector3D>

class ObjModelCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    explicit ObjModelCanvas(QWidget *parent = nullptr);
    void clearPoints() override;
    void loadObjFile(const QString &filePath);
    void resetView();
    
protected:
    void drawGrid(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawHoverIndicator(QPainter &painter) override;
    void drawCurves(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    
private:
    struct Face {
        std::vector<int> vertexIndices;
    };
    
    struct Model {
        std::vector<Eigen::Vector3f> vertices;
        std::vector<Face> faces;
    };
    void adjustCameraPosition(float a);
    void parseObjFile(const QString &filePath);
    void fitObjectToView();
    void calculateBoundingBox();
    void adjustCamera();
    Eigen::Matrix4f getModelViewProjection() const;
    QPointF projectVertex(const Eigen::Vector3f& vertex) const;

    Eigen::Vector3f m_minBound; // 包围盒最小值
    Eigen::Vector3f m_maxBound; // 包围盒最大值
    Eigen::Vector3f m_center;   // 物体中心点
    Eigen::Matrix4f m_projection; // 投影矩阵
    float m_boundingRadius; // 包围球半径
    Eigen::Vector3f m_upVector; // 相机向上向量

    Model model;
    Eigen::Vector3f modelCenter = Eigen::Vector3f::Zero();
    float modelScale = 1.0f;
    
    // 视图参数
    Eigen::Vector3f cameraPosition = Eigen::Vector3f(0, 0, 5.0f);
    Eigen::Vector3f cameraTarget = Eigen::Vector3f(0, 0, 0);
    Eigen::Vector3f cameraUp = Eigen::Vector3f(0, 1, 0);
    float fov = 45.0f;
    float zoom = 1.0f;
    
    // 旋转控制
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    QPoint lastMousePos;
};

#endif // OBJMODELCANVAS_H