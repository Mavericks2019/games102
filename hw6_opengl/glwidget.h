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
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

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
        CotangentWithArea, // 新增的带面积迭代方法
        EigenSparseSolver  // 新增Eigen稀疏求解器方法
    };
        // 添加边界类型枚举
    enum BoundaryType {
        Rectangle,
        Circle
    };
    BoundaryType boundaryType = Rectangle; // 当前边界类型
    void setBoundaryType(BoundaryType type); // 设置边界类型
    void performParameterization(); // 执行参数化
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();
    void setSurfaceColor(const QVector3D& color);
    void setSpecularEnabled(bool enabled);
    // 公共接口
    void performLoopSubdivision();
    void performMeshSimplification(float ratio);
    void setSubdivisionLevel(int level) {
        subdivisionLevel = level;
    };
    void setSimplificationRatio(float ratio) {
        simplificationRatio = ratio;
    };
    int getSubdivisionLevel() const { return subdivisionLevel; }
    void resetView(); // 现在会重置网格和视角
    void resetLoopSubdivision();
    int getCurrentSubdivisionLevel() const { return subdivisionLevel; } // 新增获取当前细分级别的方法
    void setBackgroundColor(const QColor& color);
    void setRenderMode(RenderMode mode);
    void loadOBJ(const QString &path);
    void performMinimalSurfaceIteration(int iterations, float lambda);
    void performUniformLaplacianIteration(int iterations, float lambda);
    void performCotangentWeightsIteration(int iterations, float lambda);
    void performCotangentWithAreaIteration(int iterations, float lambda);
    void performLoopSubdivisionOrigin();
    void setHideFaces(bool hide);  // 新增函数
    IterationMethod iterationMethod = UniformLaplacian; // 默认使用余切权重
    void performEigenSparseSolverIteration();  // 新增：使用Eigen求解稀疏方程组
    void setIterationMethod(IterationMethod method) { iterationMethod = method; }
        
    void applyMeshOperation(int sliderValue);  // 新增：处理滑动条操作
    void resetMeshOperation();  // 新增：重置网格操作

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
    QVector3D surfaceColor = QVector3D(1.0f, 1.0f, 0.0f); // 默认表面颜色
    bool specularEnabled = true; // 是否启用高光
    
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
    int subdivisionLevel = 0;    // 当前细分级别（0表示未细分）
    bool modelLoaded;
    QColor bgColor;
    RenderMode currentRenderMode;
    
    // 鼠标交互
    bool isDragging;
    QPoint lastMousePos;

public:
    Mesh openMesh; // 使用OpenMesh管理网格数据
    Mesh originalMesh;  // 存储原始网格
    bool hasOriginalMesh = false; // 标记是否有原始网格
    int meshOperationValue = 50; // 当前网格操作值 (0-100)
    
    // 从OpenMesh提取的数据（用于OpenGL渲染）
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;

        // 新增成员变量
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