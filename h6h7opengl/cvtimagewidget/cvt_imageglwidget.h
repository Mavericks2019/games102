#ifndef CVTIMAGEGLWIDGET_H
#define CVTIMAGEGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector2D>
#include <QPoint>
#include <QOpenGLTexture>
#include <vector>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>
#include <unordered_map>
#include <map>

// CGAL 类型定义
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> Delaunay;
typedef K::Point_2 Point;
typedef CGAL::Delaunay_triangulation_adaptation_traits_2<Delaunay> AT;
typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2<Delaunay> AP;
typedef CGAL::Voronoi_diagram_2<Delaunay, AT, AP> VoronoiDiagram;
typedef VoronoiDiagram::Locate_result Locate_result;
typedef VoronoiDiagram::Face_handle Face_handle;
typedef VoronoiDiagram::Ccb_halfedge_circulator Ccb_halfedge_circulator;

class CVTImageGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit CVTImageGLWidget(QWidget *parent = nullptr);
    ~CVTImageGLWidget();

    // CVT 相关操作
    void generateRandomPoints(int count);
    void performLloydRelaxation();
    void resetView();
    void setShowPoints(bool show);
    void setShowVoronoiDiagram(bool show);
    void setShowDelaunay(bool show);

public:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

public:
    // CVT 数据结构
    struct CanvasData {
        std::vector<Point> points;
        Delaunay dt;
    };

    // 绘图函数
    void drawRandomPoints();
    void drawVoronoiDiagram();
    void drawDelaunayTriangles();
    void drawBackground();
    void drawWhiteImageBackground(); // 新增：绘制白色图像背景
    void drawCVTContent();
    void drawImage();    
    void loadImage(const QImage& image);
    void setShowImage(bool show);
    // 图像相关
    QImage loadedImage;
    bool showImage = true;
    QOpenGLTexture* imageTexture = nullptr;
    // 着色器
    void initializeImageShaders();
    QOpenGLShaderProgram imageProgram;
    
    // Voronoi 单元处理
    std::vector<QVector2D> clipVoronoiCellToRectangle(const std::vector<QVector2D>& cell, 
                                                     float left, float right, 
                                                     float bottom, float top);
    void computeVoronoiDiagram();

    // OpenGL 资源
    void initializeShaders();

    CanvasData canvasData;
    QOpenGLShaderProgram pointProgram;
    QOpenGLVertexArrayObject pointVao;
    QOpenGLBuffer pointVbo;
    bool isCVTView = true;

    // 显示控制
    bool showPoints = true;
    bool showVoronoiDiagram = false;
    bool showDelaunay = false;
    int currentPointCount = 0;

    // 视图控制
    float rotationX = 0;
    float rotationY = 0;
    float zoom = 1.0f;
    bool isDragging = false;
    QPoint lastMousePos;

    // Voronoi 数据
    std::vector<std::vector<QVector2D>> voronoiCells;
};

#endif // CVTIMAGEGLWIDGET_H