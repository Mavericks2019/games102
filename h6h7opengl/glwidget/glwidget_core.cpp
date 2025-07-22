#include "glwidget.h"
#include <QFile>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QVector3D>
#include <QtMath>
#include <QResource>
#include <algorithm>
#include <iostream>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Tools/Smoother/JacobiLaplaceSmootherT.hh>
#include <OpenMesh/Core/IO/MeshIO.hh> // 添加OpenMesh IO头文件
#include <OpenMesh/Core/IO/Options.hh>
#include <unordered_set>
#include <Eigen/Dense>
#include <QImage> // 用于生成棋格纹理
// 在文件顶部添加这两个头文件
#include <QPainter>
#include <QFont>

using namespace OpenMesh;

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    texCoordBuffer(QOpenGLBuffer::VertexBuffer), // 初始化纹理坐标缓冲区
    pointVbo(QOpenGLBuffer::VertexBuffer), // 初始化点VBO
    showWireframeOverlay(false),
    hideFaces(false),  // 初始化新增成员
    isCVTView(false)   // 新增：CVT视图标志初始化
{
    // 设置多重采样格式
    QSurfaceFormat format;
    format.setSamples(4); // 4x MSAA
    setFormat(format);
    
    setFocusPolicy(Qt::StrongFocus);
    rotationX = rotationY = 0;
    zoom = 1.0f;
    modelLoaded = false;
    isDragging = false;
    bgColor = QColor(0, 0, 0); // 初始化背景色为黑色
    currentRenderMode = BlinnPhong;  // 默认改为实体模式
    wireframeColor = QVector4D(1.0f, 0.0f, 0.0f, 1.0f); // 红色线框
    meshOperationValue = 50;  // 默认居中
    subdivisionLevel = 0; // 确保初始化为0

}

void GLWidget::setCVTView(bool enabled)
{
    isCVTView = enabled;
    update();
}

void GLWidget::setHideFaces(bool hide)
{
    hideFaces = hide;
    update();
}

void GLWidget::setShowWireframeOverlay(bool show)
{
    showWireframeOverlay = show;
    update();
}

void GLWidget::setWireframeColor(const QVector4D& color)
{
    wireframeColor = color;
    update();
}

GLWidget::~GLWidget()
{
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
    faceEbo.destroy();
    texCoordBuffer.destroy(); // 销毁纹理坐标缓冲区
    if (checkerboardTexture) delete checkerboardTexture; // 删除纹理对象
    
    // 销毁点绘制资源
    pointVao.destroy();
    pointVbo.destroy();
    
    doneCurrent();
}

void GLWidget::resetView()
{
    rotationX = rotationY = 0;
    zoom = 1.0f;
    update();
}

void GLWidget::setBackgroundColor(const QColor& color)
{
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void GLWidget::setRenderMode(RenderMode mode)
{
    currentRenderMode = mode;
    if (modelLoaded) {
        // 重新计算曲率
        calculateCurvatures();
        
        makeCurrent();
        initializeShaders();  // 更新着色器和缓冲区
        doneCurrent();
    }
    update();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // 启用多重采样抗锯齿

    // 创建缓冲区和VAO
    vao.create();
    vaoTexture.create();
    vbo.create();
    ebo.create();
    faceEbo.create();
    texCoordBuffer.create(); // 创建纹理坐标缓冲区

    // 生成棋格纹理
    generateCheckerboardTexture();

    // 创建点绘制资源
    pointVao.create();
    pointVbo.create();
    
    // 初始化点绘制着色器
    pointProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
        "    gl_PointSize = 8.0;\n"  // 点的大小
        "}");
    
    pointProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.0, 0.0, 0.0, 1.0); // 黑色点\n"
        "}");
    
    if (!pointProgram.link()) {
        qWarning() << "Point shader link error:" << pointProgram.log();
    }

    initializeShaders();
}

void GLWidget::initializeShaders()
{
    // 删除旧的着色器程序
    if (wireframeProgram.isLinked()) {
        wireframeProgram.removeAllShaders();
    }

    if (blinnPhongProgram.isLinked()) {
        blinnPhongProgram.removeAllShaders();
    }
    // 曲率着色器程序
    if (curvatureProgram.isLinked()) {
        curvatureProgram.removeAllShaders();
    }
    if (loopSubdivisionProgram.isLinked()) {
        loopSubdivisionProgram.removeAllShaders();
    }

    // 初始化纹理着色器
    if (textureProgram.isLinked()) {
        textureProgram.removeAllShaders();
    }

    // 纹理顶点着色器
    if (!textureProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/texture.vert")) {
        qWarning() << "Texture vertex shader error:" << textureProgram.log();
    }
    
    // 纹理片段着色器
    if (!textureProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/texture.frag")) {
        qWarning() << "Texture fragment shader error:" << textureProgram.log();
    }
    
    if (!textureProgram.link()) {
        qWarning() << "Texture shader link error:" << textureProgram.log();
    }
    
    
    if (!curvatureProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/curvature.vert")) {
        qWarning() << "Curvature vertex shader error:" << curvatureProgram.log();
    }
    
    if (!curvatureProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/curvature.frag")) {
        qWarning() << "Curvature fragment shader error:" << curvatureProgram.log();
    }
    
    if (!curvatureProgram.link()) {
        qWarning() << "Curvature shader link error:" << curvatureProgram.log();
    }

    // 线框着色器程序
    if (!wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/wireframe.vert")) {
        qWarning() << "Vertex shader error:" << wireframeProgram.log();
    }
    
    if (!wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/wireframe.frag")) {
        qWarning() << "Fragment shader error:" << wireframeProgram.log();
    }
    
    if (!wireframeProgram.link()) {
        qWarning() << "Shader link error:" << wireframeProgram.log();
    }
    
    // 布林冯着色器程序
    if (!blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/blinnphong.vert")) {
        qWarning() << "Blinn-Phong vertex shader error:" << blinnPhongProgram.log();
    }
    
    if (!blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/blinnphong.frag")) {
        qWarning() << "Blinn-Phong fragment shader error:" << blinnPhongProgram.log();
    }
    
    if (!blinnPhongProgram.link()) {
        qWarning() << "Blinn-Phong shader link error:" << blinnPhongProgram.log();
    }

    // 初始化Loop细分着色器
    if (!loopSubdivisionProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/loop_subdivision.vert")) {
        qWarning() << "Loop subdivision vertex shader error:" << loopSubdivisionProgram.log();
    }
    
    if (!loopSubdivisionProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/loop_subdivision.frag")) {
        qWarning() << "Loop subdivision fragment shader error:" << loopSubdivisionProgram.log();
    }
    
    if (!loopSubdivisionProgram.link()) {
        qWarning() << "Loop subdivision shader link error:" << loopSubdivisionProgram.log();
    }

    // 如果模型已加载，更新缓冲区
    if (modelLoaded) {
        updateBuffersFromOpenMesh();
    }
}

void GLWidget::updateBuffersFromOpenMesh()
{
    if (openMesh.n_vertices() == 0) return;
    
    // 准备顶点数据 - 按顶点索引顺序存储
    std::vector<float> vertices(openMesh.n_vertices() * 3);
    std::vector<float> normals(openMesh.n_vertices() * 3);
    std::vector<float> curvatures(openMesh.n_vertices());
    for (auto vh : openMesh.vertices()) {
        int idx = vh.idx();
        const auto& p = openMesh.point(vh);
        vertices[idx*3]   = p[0];
        vertices[idx*3+1] = p[1];
        vertices[idx*3+2] = p[2];
        
        const auto& n = openMesh.normal(vh);
        normals[idx*3]   = n[0];
        normals[idx*3+1] = n[1];
        normals[idx*3+2] = n[2];
        
        curvatures[idx] = openMesh.data(vh).curvature;
    }
    
    // 绑定主VAO (用于非纹理渲染)
    vao.bind();
    vbo.bind();
    
    // 分配缓冲区
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    int curvatureSize = curvatures.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize + curvatureSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    vbo.write(vertexSize + normalSize, curvatures.data(), curvatureSize);
    
    // 设置线框着色器属性
    wireframeProgram.bind();
    int posLoc = wireframeProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        wireframeProgram.enableAttributeArray(posLoc);
        wireframeProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    } else {
        qWarning() << "Failed to find attribute location for aPos in wireframe shader";
    }
    
    // 设置布林冯着色器属性
    blinnPhongProgram.bind();
    posLoc = blinnPhongProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        blinnPhongProgram.enableAttributeArray(posLoc);
        blinnPhongProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    int normalLoc = blinnPhongProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        blinnPhongProgram.enableAttributeArray(normalLoc);
        blinnPhongProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }

    // 设置曲率着色器属性
    curvatureProgram.bind();
    posLoc = curvatureProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        curvatureProgram.enableAttributeArray(posLoc);
        curvatureProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = curvatureProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        curvatureProgram.enableAttributeArray(normalLoc);
        curvatureProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }
    
    int curvatureLoc = curvatureProgram.attributeLocation("aCurvature");
    if (curvatureLoc != -1) {
        curvatureProgram.enableAttributeArray(curvatureLoc);
        curvatureProgram.setAttributeBuffer(curvatureLoc, GL_FLOAT, vertexSize + normalSize, 1, sizeof(float));
    } else {
        qWarning() << "Failed to find attribute location for aCurvature in curvature shader";
    }
    
    // 设置Loop细分着色器属性 (使用主VAO)
    loopSubdivisionProgram.bind();
    posLoc = loopSubdivisionProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        loopSubdivisionProgram.enableAttributeArray(posLoc);
        loopSubdivisionProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = loopSubdivisionProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        loopSubdivisionProgram.enableAttributeArray(normalLoc);
        loopSubdivisionProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }

    ebo.bind();
    ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int));
    
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
    
    vao.release();
    
    // ===== 设置纹理专用VAO =====
    vaoTexture.bind();
    vbo.bind(); // 共享同一个顶点缓冲区
    
    // 设置纹理着色器属性
    textureProgram.bind();
    posLoc = textureProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        textureProgram.enableAttributeArray(posLoc);
        textureProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = textureProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        textureProgram.enableAttributeArray(normalLoc);
        textureProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }

    // 更新纹理坐标缓冲区
    updateTextureCoordinates();
    
    // 绑定纹理坐标缓冲区
    texCoordBuffer.bind();
    texCoordBuffer.allocate(texCoords.data(), texCoords.size() * sizeof(float));
    
    int texCoordLoc = textureProgram.attributeLocation("aTexCoord");
    if (texCoordLoc != -1) {
        textureProgram.enableAttributeArray(texCoordLoc);
        textureProgram.setAttributeBuffer(texCoordLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    } else {
        qWarning() << "Failed to find attribute location for aTexCoord in texture shader";
    }

    // 绑定面索引缓冲区
    faceEbo.bind();
    
    vaoTexture.release();
    textureProgram.release();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::updateTextureCoordinates()
{
        // 如果已有参数化纹理坐标，则直接使用
    if (hasParamTexCoords && !paramTexCoords.empty()) {
        texCoords = paramTexCoords;
        return;
    }
    
    // 否则使用默认计算方式
    texCoords.clear();
    texCoords.reserve(openMesh.n_vertices() * 2);
    texCoords.clear();
    texCoords.reserve(openMesh.n_vertices() * 2);
    
    for (auto vh : openMesh.vertices()) {
        const auto& p = openMesh.point(vh);
        // 将顶点坐标映射到[0,1]范围作为纹理坐标
        texCoords.push_back((p[0] + 1.0f) * 0.5f);
        texCoords.push_back((p[1] + 1.0f) * 0.5f);
    }
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 如果是CVT视图，绘制白色正方形
    if (isCVTView) {
        drawCVTBackground();
        return; // 提前返回，不绘制其他内容
    }

    if (!modelLoaded || openMesh.n_vertices() == 0) {
        return;
    }

    // 设置变换矩阵
    QMatrix4x4 model, view, projection;
    model.translate(0, 0, -2.5);
    model.rotate(rotationX, 1, 0, 0);
    model.rotate(rotationY, 0, 1, 0);
    model.scale(zoom);
    
    view.lookAt(QVector3D(0, 0, 5), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);

    if (hideFaces) {
        drawWireframe(model, view, projection);
    } else {
        switch (currentRenderMode) {
        case TextureMapping:
            drawTextureMapping(model, view, projection, normalMatrix);
            break;
        case LoopSubdivision:
            drawLoopSubdivision(model, view, projection, normalMatrix);
            break;
        case MeshSimplification:
            drawMeshSimplification(model, view, projection, normalMatrix);
            break;
        case GaussianCurvature:
        case MeanCurvature:
        case MaxCurvature:
            drawCurvature(model, view, projection, normalMatrix);
            break;
        default: // BlinnPhong and others
            drawBlinnPhong(model, view, projection, normalMatrix);
            break;
        }

        if (showWireframeOverlay) {
            drawWireframeOverlay(model, view, projection);
        }
    }
    
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (isCVTView || isParameterizationView) return;

    switch (event->key()) {
    case Qt::Key_Left:
        rotationY -= 5;
        break;
    case Qt::Key_Right:
        rotationY += 5;
        break;
    case Qt::Key_Up:
        rotationX -= 5;
        break;
    case Qt::Key_Down:
        rotationX += 5;
        break;
    case Qt::Key_Plus:
        zoom *= 1.1;
        break;
    case Qt::Key_Minus:
        zoom /= 1.1;
        break;
    case Qt::Key_R: // 重置视图
        resetView();
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
    }
    update();
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    // 如果是参数化视图，只允许缩放，不允许旋转
    if (isCVTView || isParameterizationView) return;
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (isParameterizationView) return;
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 如果是参数化视图，只允许缩放，不允许旋转
    if (isCVTView || isParameterizationView) return;
    if (isDragging) {
        QPoint currentPos = event->pos();
        QPoint delta = currentPos - lastMousePos;
        
        // 根据鼠标移动距离计算旋转角度
        rotationY += delta.x() * 0.5f;
        rotationX += delta.y() * 0.5f;
        
        // 限制X轴旋转范围在-90到90度之间
        rotationX = qBound(-90.0f, rotationX, 90.0f);
        
        lastMousePos = currentPos;
        update();
    }
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        float delta = numDegrees.y() > 0 ? 1.1f : 0.9f;
        zoom *= delta;
        
        // 限制缩放范围
        zoom = qBound(0.1f, zoom, 10.0f);
        
        update();
    }
}

// 添加新函数的实现
void GLWidget::setSurfaceColor(const QVector3D& color)
{
    surfaceColor = color;
    update();
}

void GLWidget::setSpecularEnabled(bool enabled)
{
    specularEnabled = enabled;
    update();
}

void GLWidget::resetLoopSubdivision()
{
    if (!hasOriginalMesh) return;
    
    // 恢复原始网格
    openMesh = originalMesh;
    
    // 重置细分级别
    subdivisionLevel = 0;
    
    // 更新网格数据
    updateBuffersFromOpenMesh();
    update();
}

void GLWidget::setBoundaryType(BoundaryType type) {
    boundaryType = type;
}

// 实现centerView函数
void GLWidget::centerView()
{
    if (openMesh.n_vertices() == 0) return;
    
    // 计算网格的边界框
    Mesh::Point min, max;
    min = max = openMesh.point(*openMesh.vertices_begin());
    for (auto vh : openMesh.vertices()) {
        min.minimize(openMesh.point(vh));
        max.maximize(openMesh.point(vh));
    }
    
    // 计算中心点
    Mesh::Point center = (min + max) * 0.5f;
    
    // 计算最大尺寸
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    // 设置合适的缩放级别
    zoom = 1.0f;
    if (maxSize > 0.1f) {
        zoom = 1.0f / maxSize;
    }
    
    // 重置旋转角度
    rotationX = 0;
    rotationY = 0;
    
    update();
}

void GLWidget::generateCheckerboardTexture()
{
    const int size = 512;
    const int tileSize = 32;  // 每个格子的大小
    QImage image(size, size, QImage::Format_RGB32);
    
    // 设置棕色和白色
    const QColor brownColor(139, 69, 19);  // RGB for saddle brown
    const QColor whiteColor(255, 255, 255);
    
    // 创建QPainter绘制文本
    QPainter painter(&image);
    QFont font;
    font.setFamily("Arial");
    font.setPixelSize(tileSize / 2);  // 字体大小为格子的一半
    font.setBold(true);
    painter.setFont(font);
    
    // 字母序列 (A-Z)
    const QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    // 创建棋格图案并添加字母
    for (int y = 0; y < size; y += tileSize) {
        for (int x = 0; x < size; x += tileSize) {
            // 确定当前格子颜色
            bool isBrown = ((x / tileSize) % 2) ^ ((y / tileSize) % 2);
            QColor tileColor = isBrown ? brownColor : whiteColor;
            
            // 填充整个格子
            for (int dy = 0; dy < tileSize && y + dy < size; dy++) {
                for (int dx = 0; dx < tileSize && x + dx < size; dx++) {
                    image.setPixel(x + dx, y + dy, tileColor.rgb());
                }
            }
            
            // 计算当前格子索引
            int tileIndex = (y / tileSize) * (size / tileSize) + (x / tileSize);
            char currentChar = letters.at(tileIndex % letters.length()).toLatin1();
            
            // 设置文本颜色（对比色）
            painter.setPen(isBrown ? whiteColor : brownColor);
            
            // 在格子中心绘制字母
            QRect tileRect(x, y, tileSize, tileSize);
            painter.drawText(tileRect, Qt::AlignCenter, QString(currentChar));
        }
    }
    
    painter.end();  // 结束绘制
    
    // 创建OpenGL纹理
    if (checkerboardTexture) delete checkerboardTexture;
    checkerboardTexture = new QOpenGLTexture(image.mirrored());
    checkerboardTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    checkerboardTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    checkerboardTexture->setWrapMode(QOpenGLTexture::Repeat);
}

// glwidget_core.cpp
void GLWidget::setParameterizationTexCoords(const std::vector<float>& coords) {
    paramTexCoords = coords;
    hasParamTexCoords = true;
    
    // 更新纹理坐标缓冲区
    makeCurrent();
    texCoordBuffer.bind();
    texCoordBuffer.allocate(paramTexCoords.data(), paramTexCoords.size() * sizeof(float));
    doneCurrent();
    update();
}

// 实现拆分后的辅助函数
void GLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
    wireframeProgram.bind();
    vao.bind();
    ebo.bind();

    glLineWidth(1.5f);
    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", wireframeColor);

    glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
    
    ebo.release();
    vao.release();
    wireframeProgram.release();
}

void GLWidget::drawTextureMapping(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    textureProgram.bind();
    vaoTexture.bind();
    faceEbo.bind();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    textureProgram.setUniformValue("model", model);
    textureProgram.setUniformValue("view", view);
    textureProgram.setUniformValue("projection", projection);
    textureProgram.setUniformValue("normalMatrix", normalMatrix);
    
    if (checkerboardTexture) {
        checkerboardTexture->bind(0);
        textureProgram.setUniformValue("textureSampler", 0);
    }
    
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    textureProgram.release();
}

void GLWidget::drawLoopSubdivision(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    loopSubdivisionProgram.bind();
    vao.bind();
    
    loopSubdivisionProgram.setUniformValue("model", model);
    loopSubdivisionProgram.setUniformValue("view", view);
    loopSubdivisionProgram.setUniformValue("projection", projection);
    loopSubdivisionProgram.setUniformValue("normalMatrix", normalMatrix);
    
    glDrawElements(GL_TRIANGLES, loopSubdividedMesh.indices.size(), 
                GL_UNSIGNED_INT, loopSubdividedMesh.indices.data());
    
    vao.release();
    loopSubdivisionProgram.release();
}

void GLWidget::drawMeshSimplification(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    blinnPhongProgram.bind();
    vao.bind();
    
    blinnPhongProgram.setUniformValue("model", model);
    blinnPhongProgram.setUniformValue("view", view);
    blinnPhongProgram.setUniformValue("projection", projection);
    blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
    blinnPhongProgram.setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    blinnPhongProgram.setUniformValue("viewPos", QVector3D(0.0f, 0.0f, 5.0f));
    blinnPhongProgram.setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
    
    glDrawElements(GL_TRIANGLES, simplifiedMesh.indices.size(), 
                GL_UNSIGNED_INT, simplifiedMesh.indices.data());
    
    vao.release();
    blinnPhongProgram.release();
}

void GLWidget::drawCurvature(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    curvatureProgram.bind();
    vao.bind();
    faceEbo.bind();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    curvatureProgram.setUniformValue("model", model);
    curvatureProgram.setUniformValue("view", view);
    curvatureProgram.setUniformValue("projection", projection);
    curvatureProgram.setUniformValue("normalMatrix", normalMatrix);
    curvatureProgram.setUniformValue("curvatureType", static_cast<int>(currentRenderMode));
    
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    curvatureProgram.release();
}

void GLWidget::drawBlinnPhong(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    blinnPhongProgram.bind();
    vao.bind();
    faceEbo.bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    blinnPhongProgram.setUniformValue("model", model);
    blinnPhongProgram.setUniformValue("view", view);
    blinnPhongProgram.setUniformValue("projection", projection);
    blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
    blinnPhongProgram.setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    blinnPhongProgram.setUniformValue("viewPos", QVector3D(0.0f, 0.0f, 5.0f));
    blinnPhongProgram.setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
    blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled);

    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

    faceEbo.release();
    vao.release();
    blinnPhongProgram.release();
}

void GLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0, -1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.5f);
    
    wireframeProgram.bind();
    vao.bind();
    ebo.bind();

    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", wireframeColor);

    glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
    
    ebo.release();
    vao.release();
    wireframeProgram.release();
    glDisable(GL_POLYGON_OFFSET_LINE);
}
