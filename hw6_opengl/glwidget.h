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
#include <OpenMesh/Core/IO/MeshIO.hh> // 添加OpenMesh IO头文件

// 定义OpenMesh网格类型
struct MyTraits : public OpenMesh::DefaultTraits {
    VertexAttributes(OpenMesh::Attributes::Normal | 
                     OpenMesh::Attributes::Status);
    FaceAttributes(OpenMesh::Attributes::Normal | 
                   OpenMesh::Attributes::Status);
    // 添加曲率属性
    VertexTraits {
        float curvature;
    };
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
    void calculateCurvatures();
    void setShowWireframeOverlay(bool show);
    void setWireframeColor(const QVector4D& color);
    Mesh::Point computeMeanCurvatureVector( const Mesh::VertexHandle& vh);
    float triangleArea(const Mesh::Point& p0, const Mesh::Point& p1, const Mesh::Point& p2);
    float calculateMixedArea(const Mesh::VertexHandle& vh);
    void updateBuffersFromOpenMesh(); // 新增函数
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
    Mesh openMesh; // 使用OpenMesh管理网格数据
    
    // 从OpenMesh提取的数据（用于OpenGL渲染）
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;
};

#endif // GLWIDGET_H