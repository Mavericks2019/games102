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
    
    // OpenGL资源
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

    // 视图参数
    float rotationX, rotationY;
    float zoom;
    bool modelLoaded;
    QColor bgColor;
    RenderMode currentRenderMode;
    
    // 鼠标交互
    bool isDragging;
    QPoint lastMousePos;
};

#endif // GLWIDGET_H