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
#include <queue>

#define EPSILON 1E-4F 

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
        BlinnPhong, 
        GaussianCurvature,
        MeanCurvature,
        MaxCurvature,
        LoopSubdivision,  // 新增Loop细分模式
        MeshSimplification // 新增网格简化模式
    };
    enum IterationMethod {
        UniformLaplacian,
        CotangentWeights,
        CotangentWithArea // 新增的带面积迭代方法
    };
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

    // 公共接口
    void performLoopSubdivision();
    void performMeshSimplification(float ratio);
    void setSubdivisionLevel(int level) {
        subdivisionLevel = level;
    };
    void setSimplificationRatio(float ratio) {
        simplificationRatio = ratio;
    };

    void resetView();
    void setBackgroundColor(const QColor& color);
    void setRenderMode(RenderMode mode);
    void loadOBJ(const QString &path);
    void performMinimalSurfaceIteration(int iterations, float lambda);
    void performUniformLaplacianIteration(int iterations, float lambda);
    void performCotangentWeightsIteration(int iterations, float lambda);
    void performCotangentWithAreaIteration(int iterations, float lambda);
    void setHideFaces(bool hide);  // 新增函数
    IterationMethod iterationMethod = UniformLaplacian; // 默认使用余切权重
    void setIterationMethod(IterationMethod method) { iterationMethod = method; }

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
    float cotangent(const Mesh::Point& a, const Mesh::Point& b, const Mesh::Point& c);
    float calculateMixedArea(const Mesh::VertexHandle& vh);
    void updateBuffersFromOpenMesh(); // 新增函数
    bool showWireframeOverlay;
    bool hideFaces;  // 新增成员变量
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

public:
    Mesh openMesh; // 使用OpenMesh管理网格数据
    
    // 从OpenMesh提取的数据（用于OpenGL渲染）
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;

        // 新增成员变量
    int subdivisionLevel = 1;    // Loop细分级别
    float simplificationRatio = 0.5f; // 网格简化比例
    
    // 新增OpenGL资源
    QOpenGLShaderProgram loopSubdivisionProgram;
    
    // 新增网格数据结构
    struct LoopMesh {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<unsigned int> indices;
    };
    
    LoopMesh loopSubdividedMesh;  // Loop细分后的网格
    LoopMesh simplifiedMesh;      // 简化后的网格
};

#endif // GLWIDGET_H