#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <vector>
#include <set>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

// 定义OpenMesh网格类型
struct MyTraits : public OpenMesh::DefaultTraits {
    VertexAttributes(OpenMesh::Attributes::Normal | 
                     OpenMesh::Attributes::Status);
    FaceAttributes(OpenMesh::Attributes::Normal | 
                   OpenMesh::Attributes::Status);
};
typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits> Mesh;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    enum RenderMode { 
        Wireframe, 
        BlinnPhong, 
        GaussianCurvature,
        MeanCurvature,
        MaxCurvature 
    };
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

    // 公共接口
    void resetView();
    void setBackgroundColor(const QColor& color);
    void setRenderMode(RenderMode mode);
    void loadOBJ(const QString &path);
    void performMinimalSurfaceIteration(int iterations, float lambda);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

public:
    void initializeShaders();
    void calculateNormals();
    void calculateCurvatures();
    void calculateCurvaturesOpenMesh();
    void setShowWireframeOverlay(bool show);
    void setWireframeColor(const QVector4D& color);
    bool showWireframeOverlay;
    // OpenGL资源
    QVector4D wireframeColor;
    QOpenGLShaderProgram wireframeProgram;
    QOpenGLShaderProgram blinnPhongProgram;
    QOpenGLShaderProgram curvatureProgram;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QOpenGLBuffer faceEbo;

    // 模型数据
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;
    std::set<uint64_t> uniqueEdges;

    // 曲率数据
    std::vector<float> gaussianCurvatures;
    std::vector<float> meanCurvatures;
    std::vector<float> maxCurvatures;
    
    // 顶点曲率值
    std::vector<float> vertexCurvatures;

    // 视图参数
    float rotationX, rotationY;
    float zoom;
    bool modelLoaded;
    QColor bgColor;
    RenderMode currentRenderMode;
    
    // 鼠标交互
    bool isDragging;
    QPoint lastMousePos;

private:
    Mesh openMesh; // 使用OpenMesh替代原有HMesh
    
    // 极小曲面迭代相关
    std::vector<float> originalVertices; // 保存原始顶点位置
};

#endif // GLWIDGET_H