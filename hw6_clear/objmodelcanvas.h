#ifndef OBJMODELCANVAS_H
#define OBJMODELCANVAS_H

#include "basecanvaswidget.h"
#include <Eigen/Dense>
#include <vector>
#include <QVector3D>
#include <opencv2/opencv.hpp>
#include <QMetaType>

class ObjModelCanvas : public BaseCanvasWidget
{
    Q_OBJECT
public:
    // 绘制模式枚举
    enum DrawMode {
        Triangles,  // 原有三角形填充模式
        Pixels      // 新增像素绘制模式
    };
    
    explicit ObjModelCanvas(QWidget *parent = nullptr);
    void clearPoints() override;
    void loadObjFile(const QString &filePath);
    void resetView();
    
    void setShowFaces(bool show) { 
        showFaces = show; 
        update(); 
    }
    
    // 设置绘制模式
    void setDrawMode(DrawMode mode) {
        drawMode = mode;
        update();
    }
    
    // 设置背景颜色（覆盖基类方法）
    
public:
    void drawGrid(QPainter &painter) override;
    void drawPoints(QPainter &painter) override;
    void drawHoverIndicator(QPainter &painter) override;
    void drawCurves(QPainter &painter) override;
    void drawInfoPanel(QPainter &painter) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void drawTriangles(QPainter &painter);
    
public:
    struct Face {
        std::vector<int> vertexIndices;
        Eigen::Vector3f normal;     // 面的法向量
        Eigen::Vector3f viewCenter; // 在视图空间中的中心位置
        float depth = 0.0f;         // 到相机的距离（用于深度排序）
        QColor color;               // 计算好的面颜色
    };
    
    struct Model {
        std::vector<Eigen::Vector3f> vertices;
        std::vector<Face> faces;
    };
    
    void adjustCameraPosition(float a);
    void parseObjFile(const QString &filePath);
    void fitObjectToView();
    void calculateBoundingBox();
    void calculateFaceNormals();  // 计算所有面的法向量
    void updateFaceColors();      // 更新所有面的颜色
    void updateFaceDepths();      // 更新所有面的深度
    void sortFacesByDepth();      // 按深度排序面
    void adjustCamera();
    Eigen::Matrix4f getModelViewProjection() const;
    Eigen::Matrix4f getModelMatrix() const;
    Eigen::Matrix4f getViewMatrix() const;  // 获取视图矩阵
    QPointF projectVertex(const Eigen::Vector3f& vertex) const;
    QColor calculateFaceColor(const Eigen::Vector3f& normal) const;  // 计算面颜色

    // 新增：像素绘制方法
    void drawPixels(QPainter &painter);
    
    Eigen::Vector3f m_minBound; // 包围盒最小值
    Eigen::Vector3f m_maxBound; // 包围盒最大值
    Eigen::Vector3f m_center;   // 物体中心点
    float m_boundingRadius;     // 包围球半径
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
    
    // 是否显示面（填充多边形）
    bool showFaces = false;
    
    // 绘制模式
    DrawMode drawMode = Triangles; // 默认为三角形模式
    
    // 光照参数
    Eigen::Vector3f lightPosition;  // 光源位置
    Eigen::Vector3f lightColor;     // 光源颜色
    Eigen::Vector3f ambientColor;   // 环境光颜色
    float ambientIntensity = 0.3f;   // 环境光强度
    float diffuseIntensity = 0.7f;   // 漫反射强度
    float specularIntensity = 0.4f;  // 镜面反射强度
    float shininess = 32.0f;        // 高光指数
    
    // 视图矩阵缓存（避免重复计算）
    mutable Eigen::Matrix4f cachedViewMatrix;
    mutable bool viewMatrixDirty = true;
    
    // 模型矩阵缓存
    mutable Eigen::Matrix4f cachedModelMatrix;
    mutable bool modelMatrixDirty = true;
    
    // 新增：用于像素模式的图像缓冲区
    cv::Mat pixelBuffer;
    
    // 深度缓冲区
    std::vector<float> depth_buf;
};

Q_DECLARE_METATYPE(ObjModelCanvas::DrawMode)

#endif // OBJMODELCANVAS_H