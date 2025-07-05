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

#define EPSILON 1E-4F 

using namespace OpenMesh;

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
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

void GLWidget::loadOBJ(const QString &path)
{
    openMesh.clear();
    faces.clear();
    edges.clear();
    modelLoaded = false;
    
    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    
    // 使用OpenMesh加载OBJ文件
    if (!OpenMesh::IO::read_mesh(openMesh, path.toStdString(), opt)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    // 请求法线属性
    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    
    // 计算边界框
    Mesh::Point min, max;
    if (openMesh.n_vertices() > 0) {
        min = max = openMesh.point(*openMesh.vertices_begin());
        for (auto vh : openMesh.vertices()) {
            min.minimize(openMesh.point(vh));
            max.maximize(openMesh.point(vh));
        }
    }
    
    // 计算中心点和缩放因子
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    float scaleFactor = 2.0f / maxSize;
    
    // 应用中心化和缩放
    for (auto vh : openMesh.vertices()) {
        Mesh::Point p = openMesh.point(vh);
        p = (p - center) * scaleFactor;
        openMesh.set_point(vh, p);
    }
    
    // 更新法线
    openMesh.update_normals();
    
    // 计算曲率
    calculateCurvatures();
    
    // 准备面索引数据 - 确保每个面提取3个顶点
    for (auto fh : openMesh.faces()) {
        // 获取面的顶点迭代器
        auto fv_it = openMesh.fv_ccwbegin(fh);
        auto fv_end = openMesh.fv_ccwend(fh);
        
        // 获取面顶点的数量
        int vertexCount = openMesh.valence(fh);
        
        if (vertexCount < 3) {
            qWarning() << "Face with less than 3 vertices, skipping";
            continue;
        }
        
        // 对于三角形面，直接添加三个顶点
        if (vertexCount == 3) {
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx());
        }
        // 对于多边形面，进行三角剖分（扇形）
        else {
            // 第一个顶点作为中心点
            unsigned int centerIdx = (*fv_it).idx();
            ++fv_it;
            
            // 第二个顶点作为起始点
            unsigned int prevIdx = (*fv_it).idx();
            ++fv_it;
            
            // 遍历剩余顶点，形成三角形扇
            for (int i = 2; i < vertexCount; i++) {
                unsigned int currentIdx = (*fv_it).idx();
                
                // 添加一个三角形：中心点、前一个点、当前点
                faces.push_back(centerIdx);
                faces.push_back(prevIdx);
                faces.push_back(currentIdx);
                
                // 更新前一个点为当前点，为下一个三角形做准备
                prevIdx = currentIdx;
                
                // 移动到下一个顶点
                ++fv_it;
            }
        }
    }
    
    // 准备边索引数据 - 使用半边确保方向一致
    std::set<std::pair<unsigned int, unsigned int>> uniqueEdges;
    for (auto heh : openMesh.halfedges()) {
        if (openMesh.is_boundary(heh) || heh.idx() < openMesh.opposite_halfedge_handle(heh).idx()) {
            unsigned int from = openMesh.from_vertex_handle(heh).idx();
            unsigned int to = openMesh.to_vertex_handle(heh).idx();
            
            // 确保小索引在前
            if (from > to) std::swap(from, to);
            
            uniqueEdges.insert({from, to});
        }
    }
    
    for (const auto& edge : uniqueEdges) {
        edges.push_back(edge.first);
        edges.push_back(edge.second);
    }
    
    qDebug() << "Loaded OBJ file:" << path;
    qDebug() << "Vertices:" << openMesh.n_vertices() << "Faces:" << openMesh.n_faces();
    qDebug() << "Edges:" << edges.size() / 2;
    qDebug() << "Model center:" << center[0] << "," << center[1] << "," << center[2];
    qDebug() << "Model size:" << maxSize;
    
    modelLoaded = true;
    
    // 更新OpenGL缓冲区
    makeCurrent();
    initializeShaders();  // 重新初始化着色器和缓冲区
    doneCurrent();
    
    // 重置视图参数
    rotationX = rotationY = 0;
    zoom = 1.0f;
    
    update(); // 触发重绘
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // 启用多重采样抗锯齿

    // 创建缓冲区和VAO - 只在这里创建一次
    vao.create();
    vbo.create();
    ebo.create();
    faceEbo.create();

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

    ebo.bind();
    ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int));
    
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
    
    vao.release();
}

void GLWidget::calculateCurvatures()
{
    if (openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
            openMesh.data(vh).curvature = 0.0f; // 边界顶点曲率设为0
            continue;
        }
        
        // 计算高斯曲率
        float angleDefect = 2 * M_PI;
        float area = 0.0f;
        
        for (auto vf_it = openMesh.vf_begin(vh); vf_it != openMesh.vf_end(vh); ++vf_it) {
            // 计算角度
            auto heh = openMesh.halfedge_handle(*vf_it);
            while (openMesh.to_vertex_handle(heh) != vh) {
                heh = openMesh.next_halfedge_handle(heh);
            }
            
            auto v1 = openMesh.point(openMesh.from_vertex_handle(heh));
            auto v2 = openMesh.point(openMesh.to_vertex_handle(heh));
            auto heh_next = openMesh.next_halfedge_handle(heh);
            auto v3 = openMesh.point(openMesh.to_vertex_handle(heh_next));
            
            auto vec1 = (v1 - v2).normalize();
            auto vec2 = (v3 - v2).normalize();
            float angle = acos(std::max(-1.0f, std::min(1.0f, dot(vec1, vec2))));
            angleDefect -= angle;
            
            // 计算三角形面积
            auto cross = (v1 - v2) % (v3 - v2);
            area += cross.length() / 6.0f;
        }
        
        float gaussianCurvature = 0.0f;
        if (area > EPSILON) {
            gaussianCurvature = angleDefect / area;
        }
        
        // 计算混合面积用于平均曲率
        float A_mixed = calculateMixedArea(vh);
        
        // 计算平均曲率向量
        Mesh::Point H = computeMeanCurvatureVector(vh);
        
        // 平均曲率值是曲率向量长度的一半
        float meanCurvature = H.length() / 2.0f;
        
        // 计算最大曲率
        float maxCurvature = gaussianCurvature + meanCurvature;
        
        // 根据当前渲染模式设置曲率
        switch (currentRenderMode) {
        case GaussianCurvature:
            openMesh.data(vh).curvature = gaussianCurvature;
            break;
        case MeanCurvature:
            openMesh.data(vh).curvature = meanCurvature;
            break;
        case MaxCurvature:
            openMesh.data(vh).curvature = maxCurvature;
            break;
        default:
            openMesh.data(vh).curvature = 0.0f;
            break;
        }
    }
    
    // 归一化曲率值到 [0,1] 范围
    auto normalize = [](std::vector<float>& values) {
        if (values.empty()) return;
        
        auto minmax = std::minmax_element(values.begin(), values.end());
        float minVal = *minmax.first;
        float maxVal = *minmax.second;
        float range = maxVal - minVal;
        
        if (range > 0) {
            for (float& val : values) {
                val = (val - minVal) / range;
            }
        }
    };
    
    // 只归一化非边界顶点
    std::vector<float> curvatures;
    for (auto vh : openMesh.vertices()) {
        if (!isBoundary[vh.idx()]) {
            curvatures.push_back(openMesh.data(vh).curvature);
        }
    }
    
    // 归一化非边界顶点
    normalize(curvatures);
    
    // 将归一化值复制回顶点
    size_t idx = 0;
    for (auto vh : openMesh.vertices()) {
        if (!isBoundary[vh.idx()]) {
            openMesh.data(vh).curvature = curvatures[idx++];
        }
        // 边界顶点保持为0
    }
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
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
        if (currentRenderMode == GaussianCurvature || 
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
            blinnPhongProgram.setUniformValue("objectColor", QVector3D(0.7f, 0.7f, 0.8f));

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
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
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

Mesh::Point GLWidget::computeMeanCurvatureVector(const Mesh::VertexHandle& vh) {
    Mesh::Point H(0, 0, 0);
    if(openMesh.is_boundary(vh)) {
        return H;
    }
    float A_mixed = calculateMixedArea(vh);
    if (A_mixed < EPSILON) {
        return H;
    }
    // 遍历邻接顶点
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        auto adjV = *vv_it;
        
        // 找到前一个顶点(pp)和后一个顶点(np)
        Mesh::VertexHandle pp, np;
        auto heh = openMesh.find_halfedge(vh, adjV);
        if (!heh.is_valid()) continue;
        
        // 获取前一个顶点(pp)
        if (!openMesh.is_boundary(heh)) {
            auto prev_heh = openMesh.prev_halfedge_handle(heh);
            pp = openMesh.from_vertex_handle(prev_heh);
        } else {
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            if (!openMesh.is_boundary(opp_heh)) {
                auto prev_opp_heh = openMesh.prev_halfedge_handle(opp_heh);
                pp = openMesh.from_vertex_handle(prev_opp_heh);
            } else {
                continue; // 无效边界
            }
        }
        
        // 获取后一个顶点(np)
        if (!openMesh.is_boundary(heh)) {
            auto next_heh = openMesh.next_halfedge_handle(heh);
            np = openMesh.to_vertex_handle(next_heh);
        } else {
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            if (!openMesh.is_boundary(opp_heh)) {
                auto next_opp_heh = openMesh.next_halfedge_handle(opp_heh);
                np = openMesh.to_vertex_handle(next_opp_heh);
            } else {
                continue; // 无效边界
            }
        }
        
        // 计算两个相邻三角形的面积
        float area1 = triangleArea(openMesh.point(vh), openMesh.point(adjV), openMesh.point(pp));
        float area2 = triangleArea(openMesh.point(vh), openMesh.point(adjV), openMesh.point(np));
        
        if (area1 > EPSILON && area2 > EPSILON) {
            // 计算余切权重
            auto vec1 = openMesh.point(adjV) - openMesh.point(pp);
            auto vec2 = openMesh.point(vh) - openMesh.point(pp);
            float cot_alpha = dot(vec1, vec2) / cross(vec1, vec2).length();
            
            auto vec3 = openMesh.point(adjV) - openMesh.point(np);
            auto vec4 = openMesh.point(vh) - openMesh.point(np);
            float cot_beta = dot(vec3, vec4) / cross(vec3, vec4).length();
            
            H += (cot_alpha + cot_beta) * (openMesh.point(vh) - openMesh.point(adjV));
        }
    }
    
    return H / (2.0f * A_mixed);
}

// 辅助函数：计算三角形面积
float GLWidget::triangleArea(const Mesh::Point& p0, const Mesh::Point& p1, const Mesh::Point& p2) {
    auto e1 = p1 - p0;
    auto e2 = p2 - p0;
    auto cross = e1 % e2;
    return cross.length() / 2.0f;
}

// 计算混合面积（参考GetAmixed函数）
float GLWidget::calculateMixedArea(const Mesh::VertexHandle& vh) {
    float A_mixed = 0.f;

    // 遍历邻接顶点（参考GetAmixed中的adjV）
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        auto adjV = *vv_it;
        
        // 找到共享边的下一个顶点np（参考GetAmixed中的np）
        Mesh::VertexHandle np;
        auto heh = openMesh.find_halfedge(vh, adjV);
        if (!heh.is_valid()) continue;
        
        if (!openMesh.is_boundary(heh)) {
            // 获取同一面上的下一个顶点
            auto next_heh = openMesh.next_halfedge_handle(heh);
            np = openMesh.to_vertex_handle(next_heh);
        } else {
            // 边界边处理：获取对边上的顶点
            auto opp_heh = openMesh.opposite_halfedge_handle(heh);
            if (!openMesh.is_boundary(opp_heh)) {
                auto next_opp_heh = openMesh.next_halfedge_handle(opp_heh);
                np = openMesh.to_vertex_handle(next_opp_heh);
            } else {
                continue; // 无效边界
            }
        }

        auto p_v = openMesh.point(vh);
        auto p_adjV = openMesh.point(adjV);
        auto p_np = openMesh.point(np);
        
        // 计算向量
        auto vec_adjV = p_adjV - p_v;
        auto vec_np = p_np - p_v;
        auto vec_adjV_np = p_np - p_adjV;
        auto vec_np_adjV = p_adjV - p_np;
        
        // 检查是否非钝角三角形（点积>=0）
        bool nonObtuse = 
            (vec_adjV | vec_np) >= 0.0f &&
            (p_v - p_adjV | p_np - p_adjV) >= 0.0f &&
            (p_v - p_np | p_adjV - p_np) >= 0.0f;
        
        if (nonObtuse) {
            // 锐角三角形处理
            float area = triangleArea(p_v, p_adjV, p_np);
            if (area > EPSILON) {
                // 计算余切权重
                float cotA = dot(vec_adjV, vec_np) / cross(vec_adjV, vec_np).length();
                float cotB = dot(vec_np, vec_adjV) / cross(vec_np, vec_adjV).length();
                
                // 计算距离平方
                float dist2_adjV = vec_adjV.sqrnorm();
                float dist2_np = vec_np.sqrnorm();
                
                A_mixed += (dist2_adjV * cotB + dist2_np * cotA) / 8.0f;
            }
        } else {
            // 钝角三角形处理
            float area = triangleArea(p_v, p_adjV, p_np);
            if (area > EPSILON) {
                // 检查顶点vh处的角是否钝角
                if ((vec_adjV | vec_np) < 0.0f) {
                    A_mixed += area / 2.0f;
                } else {
                    A_mixed += area / 4.0f;
                }
            }
        }
    }
    return A_mixed;
}

// 辅助函数：计算三角形中某个角的余切值
float GLWidget::cotangent(const Mesh::Point& a, const Mesh::Point& b, const Mesh::Point& c) {
    Mesh::Point vec1 = b - a;
    Mesh::Point vec2 = c - a;
    float dotProduct = vec1 | vec2;
    float crossNorm = (vec1 % vec2).norm();
    
    // 避免除以零
    if (fabs(crossNorm) < EPSILON) {
        return 0.0f;
    }
    
    return dotProduct / crossNorm;
}

void GLWidget::performCotangentWeightsIteration(int iterations, float lambda) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
    }
    
    // 执行迭代
    for (int iter = 0; iter < iterations; iter++) {
        // 存储每个顶点的新位置
        std::vector<Mesh::Point> newPositions(openMesh.n_vertices());
        
        // 计算每个顶点的新位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            
            // 边界顶点保持固定
            if (isBoundary[idx]) {
                newPositions[idx] = openMesh.point(vh);
                continue;
            }
            
            Mesh::Point weightedSum(0, 0, 0);
            float totalWeight = 0.0f;
            bool validWeights = false;
            
            // 遍历顶点的出站半边
            for (auto heh : openMesh.voh_range(vh)) {
                if (!heh.is_valid()) continue;
                
                auto vj = openMesh.to_vertex_handle(heh);
                
                // 获取半边所属的面
                auto fh = openMesh.face_handle(heh);
                if (!fh.is_valid()) continue;
                
                // 获取下一个半边
                auto next_heh = openMesh.next_halfedge_handle(heh);
                auto vk = openMesh.to_vertex_handle(next_heh);
                
                // 获取对面的面（如果存在）
                auto opp_heh = openMesh.opposite_halfedge_handle(heh);
                auto opp_fh = openMesh.face_handle(opp_heh);
                
                float cot_alpha = 0.0f;
                float cot_beta = 0.0f;
                
                // 计算第一个余切值（当前面）
                cot_alpha = cotangent(openMesh.point(vk), 
                                      openMesh.point(vh), 
                                      openMesh.point(vj));
                
                // 计算第二个余切值（对面）
                if (opp_fh.is_valid()) {
                    auto opp_next_heh = openMesh.next_halfedge_handle(opp_heh);
                    auto opp_vertex = openMesh.to_vertex_handle(opp_next_heh);
                    Mesh::Point vl = openMesh.point(opp_vertex);
                    
                    cot_beta = cotangent(vl, 
                                        openMesh.point(vh), 
                                        openMesh.point(vj));
                }
                
                // 计算权重
                float weight = cot_alpha + cot_beta;
                
                // 避免负权重导致不稳定
                if (weight > 0) {
                    weightedSum += weight * openMesh.point(vj);
                    totalWeight += weight;
                    validWeights = true;
                }
            }
            
            // 计算新位置
            if (validWeights && totalWeight > EPSILON) {
                Mesh::Point centroid = weightedSum / totalWeight;
                newPositions[idx] = openMesh.point(vh) + lambda * (centroid - openMesh.point(vh));
            } else {
                // 如果无法计算有效权重，使用均匀拉普拉斯作为后备
                Mesh::Point sum(0, 0, 0);
                int count = 0;
                for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                    sum += openMesh.point(*vv_it);
                    count++;
                }
                if (count > 0) {
                    newPositions[idx] = openMesh.point(vh) + lambda * (sum / count - openMesh.point(vh));
                } else {
                    newPositions[idx] = openMesh.point(vh);
                }
            }
        }
        
        // 更新顶点位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            openMesh.set_point(vh, newPositions[idx]);
        }
    }
    
}

// 实现均匀拉普拉斯方法
void GLWidget::performUniformLaplacianIteration(int iterations, float lambda) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
    }
    
    // 执行迭代
    for (int iter = 0; iter < iterations; iter++) {
        // 存储每个顶点的新位置
        std::vector<Mesh::Point> newPositions(openMesh.n_vertices());
        
        // 计算每个顶点的新位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            
            // 边界顶点保持固定
            if (isBoundary[idx]) {
                newPositions[idx] = openMesh.point(vh);
                continue;
            }
            
            // 计算邻接顶点的平均位置
            Mesh::Point sum(0, 0, 0);
            int count = 0;
            for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
                sum += openMesh.point(*vv_it);
                count++;
            }
            
            if (count > 0) {
                Mesh::Point avg = sum / count;
                newPositions[idx] = openMesh.point(vh) + lambda * (avg - openMesh.point(vh));
            } else {
                newPositions[idx] = openMesh.point(vh);
            }
        }
        
        // 更新顶点位置
        for (auto vh : openMesh.vertices()) {
            unsigned int idx = vh.idx();
            openMesh.set_point(vh, newPositions[idx]);
        }
    }
}

// 修改后的performMinimalSurfaceIteration函数
void GLWidget::performMinimalSurfaceIteration(int iterations, float lambda) {
    switch (iterationMethod) {
    case UniformLaplacian:
        performUniformLaplacianIteration(iterations, lambda);
        break;
    case CotangentWeights:
        performCotangentWeightsIteration(iterations, lambda);
        break;
    }
    
    // 更新法线
    openMesh.update_normals();
    
    // 重新计算曲率
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    update();
}