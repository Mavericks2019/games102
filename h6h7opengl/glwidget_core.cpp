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

using namespace OpenMesh;

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    texCoordBuffer(QOpenGLBuffer::VertexBuffer), // 初始化纹理坐标缓冲区
    showWireframeOverlay(false),
    hideFaces(false)  // 初始化新增成员
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

    if (!modelLoaded || openMesh.n_vertices() == 0) {
        return;
    }

    // 设置变换矩阵
    QMatrix4x4 model, view, projection;
    
    // 应用用户变换
    model.translate(0, 0, -2.5);
    model.rotate(rotationX, 1, 0, 0);
    model.rotate(rotationY, 0, 1, 0);
    model.scale(zoom);
    
    // 设置相机
    view.lookAt(QVector3D(0, 0, 5), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);

    // 如果启用了隐藏面，只绘制线框
    if (hideFaces) {
        // 使用线框着色器
        wireframeProgram.bind();
        vao.bind();
        ebo.bind();

        // 设置线条宽度
        glLineWidth(1.5f);

        wireframeProgram.setUniformValue("model", model);
        wireframeProgram.setUniformValue("view", view);
        wireframeProgram.setUniformValue("projection", projection);
        wireframeProgram.setUniformValue("lineColor", wireframeColor);

        // 绘制线框
        glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
        
        ebo.release();
        vao.release();
        wireframeProgram.release();
    } else {
        // 正常绘制模式
        if (currentRenderMode == TextureMapping) {
            // 纹理映射模式
            textureProgram.bind();
            vaoTexture.bind(); // 使用纹理专用VAO
            faceEbo.bind();
            
            // 设置多边形填充
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            // 设置变换矩阵
            textureProgram.setUniformValue("model", model);
            textureProgram.setUniformValue("view", view);
            textureProgram.setUniformValue("projection", projection);
            textureProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 绑定纹理
            if (checkerboardTexture) {
                checkerboardTexture->bind(0);
                textureProgram.setUniformValue("textureSampler", 0);
            }
            
            // 绘制模型
            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
            
            faceEbo.release();
            vao.release();
            textureProgram.release();
        }
        else if (currentRenderMode == LoopSubdivision) {
            loopSubdivisionProgram.bind();
            vao.bind();
            
            // 设置变换矩阵
            loopSubdivisionProgram.setUniformValue("model", model);
            loopSubdivisionProgram.setUniformValue("view", view);
            loopSubdivisionProgram.setUniformValue("projection", projection);
            loopSubdivisionProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 绘制细分后的网格
            glDrawElements(GL_TRIANGLES, loopSubdividedMesh.indices.size(), 
                        GL_UNSIGNED_INT, loopSubdividedMesh.indices.data());
            
            vao.release();
            loopSubdivisionProgram.release();
        }else if (currentRenderMode == MeshSimplification) {
            blinnPhongProgram.bind(); // 使用Blinn-Phong着色器
            vao.bind();
            
            // 设置变换矩阵
            blinnPhongProgram.setUniformValue("model", model);
            blinnPhongProgram.setUniformValue("view", view);
            blinnPhongProgram.setUniformValue("projection", projection);
            blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置光照参数
            blinnPhongProgram.setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
            blinnPhongProgram.setUniformValue("viewPos", QVector3D(0.0f, 0.0f, 5.0f));
            blinnPhongProgram.setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
            blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
            
            // 绘制简化后的网格
            glDrawElements(GL_TRIANGLES, simplifiedMesh.indices.size(), 
                        GL_UNSIGNED_INT, simplifiedMesh.indices.data());
            
            vao.release();
            blinnPhongProgram.release();
        }else if (currentRenderMode == GaussianCurvature || 
               currentRenderMode == MeanCurvature || 
               currentRenderMode == MaxCurvature) {
            // 曲率可视化模式
            curvatureProgram.bind();
            vao.bind();
            faceEbo.bind();
            
            // 设置多边形填充
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            // 设置变换矩阵
            curvatureProgram.setUniformValue("model", model);
            curvatureProgram.setUniformValue("view", view);
            curvatureProgram.setUniformValue("projection", projection);
            curvatureProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置曲率类型
            curvatureProgram.setUniformValue("curvatureType", static_cast<int>(currentRenderMode));
            
            // 绘制模型
            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
            
            faceEbo.release();
            vao.release();
            curvatureProgram.release();
        } else {
            // 布林冯模式
            blinnPhongProgram.bind();
            vao.bind();
            faceEbo.bind();

            // 启用多边形填充
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // 设置变换矩阵
            blinnPhongProgram.setUniformValue("model", model);
            blinnPhongProgram.setUniformValue("view", view);
            blinnPhongProgram.setUniformValue("projection", projection);
            blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置光照参数
            blinnPhongProgram.setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
            blinnPhongProgram.setUniformValue("viewPos", QVector3D(0.0f, 0.0f, 5.0f));
            blinnPhongProgram.setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
            blinnPhongProgram.setUniformValue("objectColor", surfaceColor); // 使用新表面颜色
            blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled); // 高光开关

            // 绘制模型
            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            blinnPhongProgram.release();
        }

        // 如果启用了线框叠加
        if (showWireframeOverlay) {
            // 启用多边形偏移以避免深度冲突
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0, -1.0);
            
            // 设置线框模式
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.5f);
            
            // 使用线框着色器
            wireframeProgram.bind();
            vao.bind();
            ebo.bind();

            wireframeProgram.setUniformValue("model", model);
            wireframeProgram.setUniformValue("view", view);
            wireframeProgram.setUniformValue("projection", projection);
            wireframeProgram.setUniformValue("lineColor", wireframeColor);

            // 绘制线框
            glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
            
            ebo.release();
            vao.release();
            wireframeProgram.release();
            
            // 禁用多边形偏移
            glDisable(GL_POLYGON_OFFSET_LINE);
        }
    }
    
    // 恢复原始多边形模式
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
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
    if (isParameterizationView) return;
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
    if (isParameterizationView) return;
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
    QImage image(size, size, QImage::Format_RGB32);
    
    // 创建棋格图案
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // 每16像素一个格子
            bool isBlack = ((x / 32) % 2) ^ ((y / 32) % 2);
            image.setPixel(x, y, isBlack ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
        }
    }
    
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