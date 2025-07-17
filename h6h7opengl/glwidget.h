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
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <queue>
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>

#define EPSILON 1E-4F 

// Define OpenMesh mesh type with custom traits
// 定义带有自定义特性的OpenMesh网格类型
struct MyTraits : public OpenMesh::DefaultTraits {
    VertexAttributes(OpenMesh::Attributes::Normal | 
                     OpenMesh::Attributes::Status);
    FaceAttributes(OpenMesh::Attributes::Normal | 
                   OpenMesh::Attributes::Status);
    // Add curvature attribute to vertices
    // 为顶点添加曲率属性
    VertexTraits {
        float curvature;  // Stores curvature value per vertex (每个顶点的曲率值)
    };
};
typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits> Mesh;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    // ========== ENUMERATIONS ========== //
    // Rendering visualization modes
    // 渲染可视化模式
    enum RenderMode { 
        BlinnPhong,         // Blinn-Phong shading (Blinn-Phong着色)
        GaussianCurvature,   // Gaussian curvature visualization (高斯曲率可视化)
        MeanCurvature,       // Mean curvature visualization (平均曲率可视化)
        MaxCurvature,        // Maximum curvature visualization (最大曲率可视化)
        LoopSubdivision,     // Loop subdivision surface (Loop细分曲面)
        MeshSimplification   // Mesh simplification view (网格简化视图)
    };
    
    // Iteration methods for minimal surface
    // 极小曲面迭代方法
    enum IterationMethod {
        UniformLaplacian,    // Uniform Laplacian smoothing (均匀拉普拉斯平滑)
        CotangentWeights,    // Cotangent weight smoothing (余切权重平滑)
        CotangentWithArea,   // Area-weighted cotangent smoothing (带面积加权的余切平滑)
        EigenSparseSolver    // Eigen sparse matrix solver (Eigen稀疏矩阵求解器)
    };
    
    // Boundary types for parameterization
    // 参数化边界类型
    enum BoundaryType {
        Rectangle,           // Rectangular boundary parameterization (矩形边界参数化)
        Circle               // Circular boundary parameterization (圆形边界参数化)
    };

    // ========== CONSTRUCTOR/DESTRUCTOR ========== //
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

    // ========== OPENGL OVERRIDES ========== //
protected:
    void initializeGL() override;        // Initialize OpenGL context (初始化OpenGL上下文)
    void resizeGL(int w, int h) override;// Handle window resize (处理窗口大小调整)
    void paintGL() override;             // Main rendering function (主渲染函数)
    void keyPressEvent(QKeyEvent *event) override;        // Keyboard input (键盘输入处理)
    void mousePressEvent(QMouseEvent *event) override;    // Mouse press (鼠标按下处理)
    void mouseReleaseEvent(QMouseEvent *event) override;  // Mouse release (鼠标释放处理)
    void mouseMoveEvent(QMouseEvent *event) override;     // Mouse movement (鼠标移动处理)
    void wheelEvent(QWheelEvent *event) override;         // Mouse wheel (鼠标滚轮处理)

    // ========== RENDERING CONTROL ========== //
public:
    void setRenderMode(RenderMode mode);              // Set visualization mode (设置可视化模式)
    void setBackgroundColor(const QColor& color);      // Set background color (设置背景颜色)
    void setWireframeColor(const QVector4D& color);    // Set wireframe color (设置线框颜色)
    void setSurfaceColor(const QVector3D& color);      // Set surface color (设置表面颜色)
    void setSpecularEnabled(bool enabled);             // Toggle specular highlights (切换高光效果)
    void setShowWireframeOverlay(bool show);           // Show/hide wireframe overlay (显示/隐藏线框叠加)
    void setHideFaces(bool hide);                     // Hide face rendering (show wireframe only) (隐藏面渲染，仅显示线框)
    void resetView();                                 // Reset camera position (重置相机位置)

    // ========== MESH OPERATIONS ========== //
public:
    void loadOBJ(const QString &path);                // Load OBJ file (加载OBJ文件)
    void performLoopSubdivision();                    // Apply Loop subdivision (应用Loop细分)
    void performMeshSimplification(float ratio);       // Simplify mesh (0-1 ratio) (网格简化，0-1比例)
    void applyMeshOperation(int sliderValue);          // Apply mesh op based on UI slider (根据UI滑块应用网格操作)
    void resetMeshOperation();                        // Reset to original mesh (重置到原始网格)
    void resetLoopSubdivision();                      // Reset subdivision state (重置细分状态)
    void setBoundaryType(BoundaryType type);          // Set parameterization boundary type (设置参数化边界类型)
     int getCurrentSubdivisionLevel()                // Get current subdivision level (获取当前细分级别)
     const { return subdivisionLevel; }
    void clearMeshData(); // 清除当前网格数据
    bool loadOBJToOpenMesh(const QString &path); // 加载OBJ文件到OpenMesh
    void computeBoundingBox(Mesh::Point& min, Mesh::Point& max); // 计算网格的边界框
    void centerAndScaleMesh(const Mesh::Point& center, float maxSize); // 中心化并缩放网格
    void prepareFaceIndices(); // 准备面索引数据（包括三角剖分）
    void prepareEdgeIndices(); // 准备边索引数据
    void saveOriginalMesh(); // 保存原始网格状态           

    // ========== CURVATURE & GEOMETRY ========== //
public:
    void calculateCurvatures();                       // Compute curvature values (计算曲率值)
    Mesh::Point computeMeanCurvatureVector(const Mesh::VertexHandle& vh);  // Calculate mean curvature vector (计算平均曲率向量)
    float triangleArea(const Mesh::Point& p0,         // Compute triangle area (计算三角形面积)
                      const Mesh::Point& p1, 
                      const Mesh::Point& p2);
    float cotangent(const Mesh::Point& a,             // Compute cotangent value (计算余切值)
                   const Mesh::Point& b, 
                   const Mesh::Point& c);
    float calculateMixedArea(const Mesh::VertexHandle& vh);  // Compute mixed area for curvature (计算曲率混合面积)

    // ========== MINIMAL SURFACE ========== //
public:
    void performMinimalSurfaceIteration(int iterations, float lambda);  // Smoothing iteration (平滑迭代)
    void performUniformLaplacianIteration(int iterations, float lambda); // Uniform smoothing (均匀平滑)
    void performCotangentWeightsIteration(int iterations, float lambda); // Cotangent smoothing (余切权重平滑)
    void performCotangentWithAreaIteration(int iterations, float lambda); // Area-weighted smoothing (带面积加权的平滑)
    void performEigenSparseSolverIteration();         // Solve with Eigen sparse solver (使用Eigen稀疏求解器求解)
    void setIterationMethod(IterationMethod method) { iterationMethod = method; } // Set smoothing method (设置平滑方法)
// ========== PARAMETERIZATION ========== //
public:
    void performParameterization();                   // Perform mesh parameterization (执行网格参数化)
    void mapBoundaryToCircle();                      // Map boundary to circle (映射边界到圆形)
    void mapBoundaryToRectangle();                   // Map boundary to rectangle (映射边界到矩形)
    void resetViewForParameterization()
    {
        rotationX = 0;
        rotationY = 0;
        zoom = 1.0f;
        update();
    }                                                // 新增：为参数化重置视图
    void solveParameterization();                    // Solve parameterization using Eigen (使用Eigen求解参数化)
    void generateCheckerboardTexture();              // 生成棋格纹理
    void updateTextureCoordinates();                 // 更新纹理坐标
    // ========== OPENGL RESOURCES ========== //
public:
    void initializeShaders();                         // Compile/link shaders (编译/链接着色器)
    void updateBuffersFromOpenMesh();                 // Update GPU buffers from mesh data (从网格数据更新GPU缓冲区)
    bool isParameterizationView = false;

    // ========== DATA STRUCTURES ========== //
    // Structure for storing Loop subdivision results
    // 用于存储Loop细分结果的结构
    struct LoopMesh {
        std::vector<float> vertices;      // Vertex positions (顶点位置)
        std::vector<float> normals;       // Vertex normals (顶点法线)
        std::vector<unsigned int> indices;// Face indices (面索引)
    };

    // ========== PUBLIC MEMBER VARIABLES ========== //
public:
    // Rendering state
    void centerView(); // 新增：自适应调整视图
    void normalizeMesh();
    QVector3D surfaceColor = QVector3D(1.0f, 1.0f, 0.0f);  // Surface color (表面颜色)
    bool specularEnabled = true;          // Specular highlights enabled (高光效果启用)
    QVector4D wireframeColor;             // Wireframe RGBA color (线框RGBA颜色)
    QColor bgColor;                       // Background color (背景颜色)
    RenderMode currentRenderMode;         // Current rendering mode (当前渲染模式)
    BoundaryType boundaryType = Rectangle;// Current boundary type (当前边界类型)

    // 纹理相关
    QOpenGLTexture* checkerboardTexture = nullptr; // 棋格纹理
    std::vector<float> texCoords;          // 纹理坐标
    
    // Mesh data
    Mesh openMesh;                        // Current mesh (当前网格)
    Mesh originalMesh;                    // Original unmodified mesh (原始未修改网格)
    bool hasOriginalMesh = false;         // Has original mesh been stored (是否存储了原始网格)
    int meshOperationValue = 50;          // UI mesh operation value (0-100) (UI网格操作值0-100)
    float simplificationRatio = 0.5f;     // Mesh simplification ratio (网格简化比例)
    
    // Geometry buffers
    std::vector<unsigned int> faces;      // Face indices for rendering (渲染面索引)
    std::vector<unsigned int> edges;      // Edge indices for rendering (渲染边索引)
    LoopMesh loopSubdividedMesh;          // Loop subdivision result (Loop细分结果)
    LoopMesh simplifiedMesh;              // Simplified mesh result (简化网格结果)
    
    // View parameters
    float rotationX, rotationY;           // Rotation angles (旋转角度)
    float zoom;                           // Zoom level (缩放级别)
    int subdivisionLevel = 0;             // Current subdivision level (当前细分级别)
    
    // UI state
    bool showWireframeOverlay;            // Show wireframe overlay (显示线框叠加)
    bool hideFaces;                       // Hide face rendering (隐藏面渲染)
    bool modelLoaded;                     // Is model loaded (模型是否加载)
    IterationMethod iterationMethod = UniformLaplacian; // Current iteration method (当前迭代方法)

    // ========== OPENGL OBJECTS ========== //
protected:
    QOpenGLShaderProgram wireframeProgram;    // Wireframe shader (线框着色器)
    QOpenGLShaderProgram blinnPhongProgram;   // Blinn-Phong shader (Blinn-Phong着色器)
    QOpenGLShaderProgram curvatureProgram;    // Curvature visualization shader (曲率可视化着色器)
    QOpenGLShaderProgram loopSubdivisionProgram; // Loop subdivision shader (Loop细分着色器)
    
    QOpenGLVertexArrayObject vao;         // Vertex array object (顶点数组对象)
    QOpenGLBuffer vbo;                    // Vertex buffer (顶点缓冲区)
    QOpenGLBuffer ebo;                    // Edge index buffer (边索引缓冲区)
    QOpenGLBuffer faceEbo;                // Face index buffer (面索引缓冲区)

    // ========== INTERACTION STATE ========== //
protected:
    bool isDragging;                      // Is mouse dragging (是否正在拖动鼠标)
    QPoint lastMousePos;                  // Last mouse position (最后鼠标位置)
};

#endif // GLWIDGET_H