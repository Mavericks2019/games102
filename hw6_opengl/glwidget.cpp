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

#define EPSILON 1E-4F 

using namespace OpenMesh;

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    showWireframeOverlay(false)
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
    currentRenderMode = Wireframe;
    wireframeColor = QVector4D(1.0f, 0.0f, 0.0f, 1.0f); // 红色线框
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
        // 重新计算曲率，以更新vertexCurvatures
        calculateCurvatures();
        
        makeCurrent();
        initializeShaders();  // 更新着色器和缓冲区
        doneCurrent();
    }
    update();
}

void GLWidget::loadOBJ(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << path;
        return;
    }

    vertices.clear();
    faces.clear();
    edges.clear();
    uniqueEdges.clear();
    normals.clear();
    gaussianCurvatures.clear();
    meanCurvatures.clear();
    maxCurvatures.clear();
    vertexCurvatures.clear(); // 清空曲率数据
    originalVertices.clear(); // 清空原始顶点
    modelLoaded = false;
    openMesh.clear(); // 清空OpenMesh
    
    // 临时存储原始顶点数据
    std::vector<float> rawVertices;
    
    // 用于计算边界框
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();
    
    int vertexCount = 0;
    int faceCount = 0;

    // 第一遍：读取顶点数据并计算包围盒
    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        
        QList<QByteArray> tokens = line.split(' ');
        tokens.removeAll(QByteArray()); // 移除空项
        
        if (tokens.isEmpty()) continue;

        if (tokens[0] == "v") {
            if (tokens.size() < 4) {
                qWarning() << "Invalid vertex line:" << line;
                continue;
            }
            
            float x = tokens[1].toFloat();
            float y = tokens[2].toFloat();
            float z = tokens[3].toFloat();
            
            rawVertices.push_back(x);
            rawVertices.push_back(y);
            rawVertices.push_back(z);
            
            // 更新边界框
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            minZ = std::min(minZ, z);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
            maxZ = std::max(maxZ, z);
            
            vertexCount++;
        }
    }
    
    // 计算模型中心点和大小
    QVector3D center(
        (minX + maxX) / 2.0f,
        (minY + maxY) / 2.0f,
        (minZ + maxZ) / 2.0f
    );
    
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float maxSize = std::max(std::max(sizeX, sizeY), sizeZ);
    
    // 缩放因子，使模型适配视图
    float scaleFactor = 2.0f / maxSize;
    
    // 第二遍：处理顶点和面，应用中心化和缩放
    file.reset();
    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        
        QList<QByteArray> tokens = line.split(' ');
        tokens.removeAll(QByteArray()); // 移除空项
        
        if (tokens.isEmpty()) continue;

        if (tokens[0] == "v") {
            // 顶点数据已经在第一遍处理过，这里跳过
            continue;
        } else if (tokens[0] == "f") {
            // 临时存储当前面的顶点索引
            std::vector<unsigned int> faceIndices;
            
            for (int i = 1; i < tokens.size(); i++) {
                QList<QByteArray> indices = tokens[i].split('/');
                if (!indices.isEmpty()) {
                    bool ok;
                    int idx = indices[0].toInt(&ok);
                    if (ok && idx > 0) {
                        faceIndices.push_back(idx - 1);
                    }
                }
            }
            
            // 将面索引添加到全局列表
            for (auto idx : faceIndices) {
                faces.push_back(idx);
            }
            
            // 为当前面生成边（避免重复边）
            for (size_t i = 0; i < faceIndices.size(); i++) {
                unsigned int idx1 = faceIndices[i];
                unsigned int idx2 = faceIndices[(i + 1) % faceIndices.size()];
                
                // 确保边的索引顺序一致（小索引在前）
                if (idx1 > idx2) {
                    std::swap(idx1, idx2);
                }
                
                // 创建唯一的边标识符
                uint64_t edgeKey = (static_cast<uint64_t>(idx1) << 32) | idx2;
                
                // 如果这条边尚未添加
                if (uniqueEdges.find(edgeKey) == uniqueEdges.end()) {
                    uniqueEdges.insert(edgeKey);
                    edges.push_back(idx1);
                    edges.push_back(idx2);
                }
            }
            
            faceCount++;
        }
    }
    file.close();
    uniqueEdges.clear(); // 清空临时存储
    
    // 应用变换到顶点数据
    for (size_t i = 0; i < rawVertices.size(); i += 3) {
        // 中心化
        float x = rawVertices[i] - center.x();
        float y = rawVertices[i+1] - center.y();
        float z = rawVertices[i+2] - center.z();
        
        // 保存原始顶点位置（未缩放）
        originalVertices.push_back(x);
        originalVertices.push_back(y);
        originalVertices.push_back(z);
        
        // 缩放
        x *= scaleFactor;
        y *= scaleFactor;
        z *= scaleFactor;
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
    }
    
    // 计算法线
    calculateNormals();

    // 计算曲率
    calculateCurvatures();
    
    // 初始化顶点曲率值
    vertexCurvatures.resize(vertices.size() / 3, 0.0f);
    // 注意：在calculateCurvatures中已经根据当前模式设置了vertexCurvatures
    
    qDebug() << "Loaded OBJ file:" << path;
    qDebug() << "Vertices:" << vertexCount << "Faces:" << faceCount;
    qDebug() << "Edges:" << edges.size() / 2;
    qDebug() << "Model center:" << center;
    qDebug() << "Model size:" << maxSize;
    
    modelLoaded = true;
    
    // 更新OpenGL缓冲区
    makeCurrent();
    initializeShaders();  // 只需重新初始化着色器
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

    vao.bind();
    vbo.bind();
    
    // 分配缓冲区
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    int curvatureSize = vertexCurvatures.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize + curvatureSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    vbo.write(vertexSize + normalSize, vertexCurvatures.data(), curvatureSize);
    
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
    ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int)); // 使用边数据
    vao.release();
    
    // 创建面索引缓冲区
    faceEbo.create();
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
}

void GLWidget::calculateCurvatures() {
    calculateCurvaturesOpenMesh();
}

void GLWidget::calculateCurvaturesOpenMesh() {
    gaussianCurvatures.clear();
    meanCurvatures.clear();
    maxCurvatures.clear();
    
    if (vertices.empty() || faces.empty()) return;
    
    // 构建OpenMesh网格
    openMesh.clear();
    
    // 添加顶点
    for (size_t i = 0; i < vertices.size(); i += 3) {
        openMesh.add_vertex(Mesh::Point(vertices[i], vertices[i+1], vertices[i+2]));
    }
    
    // 添加面
    for (size_t i = 0; i < faces.size(); i += 3) {
        std::vector<Mesh::VertexHandle> faceVertices;
        faceVertices.push_back(Mesh::VertexHandle(faces[i]));
        faceVertices.push_back(Mesh::VertexHandle(faces[i+1]));
        faceVertices.push_back(Mesh::VertexHandle(faces[i+2]));
        openMesh.add_face(faceVertices);
    }
    
    // 请求顶点法线
    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    
    // 计算法线
    openMesh.update_face_normals();
    openMesh.update_vertex_normals();
    
    // 计算曲率
    size_t vertexCount = vertices.size() / 3;
    gaussianCurvatures.resize(vertexCount, 0.0f);
    meanCurvatures.resize(vertexCount, 0.0f);
    maxCurvatures.resize(vertexCount, 0.0f);
    
    // 标记边界顶点
    std::vector<bool> isBoundary(vertexCount, false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
            // 边界顶点曲率设为0
            gaussianCurvatures[vh.idx()] = 0.0f;
            meanCurvatures[vh.idx()] = 0.0f;
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
        
        if (area > EPSILON) {
            gaussianCurvatures[vh.idx()] = angleDefect / area;
        }
        
        // 计算混合面积用于平均曲率
        float A_mixed = calculateMixedArea(openMesh, vh, isBoundary);
        
        // 计算平均曲率向量
        Mesh::Point H = computeMeanCurvatureVector(openMesh, vh, A_mixed);
        
        // 平均曲率值是曲率向量长度的一半
        meanCurvatures[vh.idx()] = H.length() / 2.0f;
    }
    
    // 计算最大曲率
    for (size_t i = 0; i < vertexCount; i++) {
        if (isBoundary[i]) {
            maxCurvatures[i] = 0.0f;
        } else {
            maxCurvatures[i] = gaussianCurvatures[i] + meanCurvatures[i];
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
    std::vector<float> gaussianTemp = gaussianCurvatures;
    std::vector<float> meanTemp = meanCurvatures;
    std::vector<float> maxTemp = maxCurvatures;
    
    // 从临时数组中移除边界顶点
    auto removeBoundary = [&isBoundary](std::vector<float>& values) {
        std::vector<float> nonBoundary;
        for (size_t i = 0; i < values.size(); i++) {
            if (!isBoundary[i]) {
                nonBoundary.push_back(values[i]);
            }
        }
        return nonBoundary;
    };
    
    auto nonBoundaryGaussian = removeBoundary(gaussianTemp);
    auto nonBoundaryMean = removeBoundary(meanTemp);
    auto nonBoundaryMax = removeBoundary(maxTemp);
    
    // 归一化非边界顶点
    normalize(nonBoundaryGaussian);
    normalize(nonBoundaryMean);
    normalize(nonBoundaryMax);
    
    // 将归一化值复制回原数组
    size_t idx = 0;
    for (size_t i = 0; i < gaussianCurvatures.size(); i++) {
        if (!isBoundary[i]) {
            gaussianCurvatures[i] = nonBoundaryGaussian[idx];
            meanCurvatures[i] = nonBoundaryMean[idx];
            maxCurvatures[i] = nonBoundaryMax[idx];
            idx++;
        }
        // 边界顶点保持为0
    }
    
    // 根据当前渲染模式设置顶点曲率值
    if (vertexCurvatures.size() != gaussianCurvatures.size()) {
        vertexCurvatures.resize(gaussianCurvatures.size());
    }
    for (size_t i = 0; i < vertexCurvatures.size(); i++) {
        switch (currentRenderMode) {
        case GaussianCurvature:
            vertexCurvatures[i] = gaussianCurvatures[i];
            break;
        case MeanCurvature:
            vertexCurvatures[i] = meanCurvatures[i];
            break;
        case MaxCurvature:
            vertexCurvatures[i] = maxCurvatures[i];
            break;
        default:
            vertexCurvatures[i] = 0.0f;
            break;
        }
    }
    
    // 释放法线
    openMesh.release_vertex_normals();
    openMesh.release_face_normals();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!modelLoaded || vertices.empty()) {
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

    if (currentRenderMode == Wireframe) {
        // 线框模式
        wireframeProgram.bind();
        vao.bind();

        // 设置线条宽度
        glLineWidth(1.5f);

        wireframeProgram.setUniformValue("model", model);
        wireframeProgram.setUniformValue("view", view);
        wireframeProgram.setUniformValue("projection", projection);
        wireframeProgram.setUniformValue("lineColor", wireframeColor); // 设置线框颜色

        // 绘制模型 - 使用线框模式 (GL_LINES)
        glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);

        vao.release();
        wireframeProgram.release();
    }
    else 
    {
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
            wireframeProgram.setUniformValue("lineColor", wireframeColor); // 设置线框颜色为红色

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

void GLWidget::calculateNormals()
{
    normals.resize(vertices.size(), 0.0f);
    
    // 遍历每个面（三角形）
    for (size_t i = 0; i < faces.size(); i += 3) {
        unsigned int idx1 = faces[i];
        unsigned int idx2 = faces[i+1];
        unsigned int idx3 = faces[i+2];
        
        // 获取三个顶点
        QVector3D v1(vertices[idx1*3], vertices[idx1*3+1], vertices[idx1*3+2]);
        QVector3D v2(vertices[idx2*3], vertices[idx2*3+1], vertices[idx2*3+2]);
        QVector3D v3(vertices[idx3*3], vertices[idx3*3+1], vertices[idx3*3+2]);
        
        // 计算面的法线
        QVector3D edge1 = v2 - v1;
        QVector3D edge2 = v3 - v1;
        QVector3D normal = QVector3D::crossProduct(edge1, edge2).normalized();
        
        // 将法线累加到顶点
        normals[idx1*3]   += normal.x();
        normals[idx1*3+1] += normal.y();
        normals[idx1*3+2] += normal.z();
        
        normals[idx2*3]   += normal.x();
        normals[idx2*3+1] += normal.y();
        normals[idx2*3+2] += normal.z();
        
        normals[idx3*3]   += normal.x();
        normals[idx3*3+1] += normal.y();
        normals[idx3*3+2] += normal.z();
    }
    
    // 归一化法线
    for (size_t i = 0; i < normals.size(); i += 3) {
        QVector3D n(normals[i], normals[i+1], normals[i+2]);
        n.normalize();
        normals[i]   = n.x();
        normals[i+1] = n.y();
        normals[i+2] = n.z();
    }
}


Mesh::Point GLWidget::computeMeanCurvatureVector(Mesh& mesh, const Mesh::VertexHandle& vh, float A_mixed) {
    Mesh::Point H(0, 0, 0);
    if (A_mixed < EPSILON) return H;

    // 遍历邻接顶点
    for (auto vv_it = mesh.vv_begin(vh); vv_it != mesh.vv_end(vh); ++vv_it) {
        auto adjV = *vv_it;
        
        // 找到前一个顶点(pp)和后一个顶点(np)
        Mesh::VertexHandle pp, np;
        auto heh = mesh.find_halfedge(vh, adjV);
        if (!heh.is_valid()) continue;
        
        // 获取前一个顶点(pp)
        if (!mesh.is_boundary(heh)) {
            auto prev_heh = mesh.prev_halfedge_handle(heh);
            pp = mesh.from_vertex_handle(prev_heh);
        } else {
            auto opp_heh = mesh.opposite_halfedge_handle(heh);
            if (!mesh.is_boundary(opp_heh)) {
                auto prev_opp_heh = mesh.prev_halfedge_handle(opp_heh);
                pp = mesh.from_vertex_handle(prev_opp_heh);
            } else {
                continue; // 无效边界
            }
        }
        
        // 获取后一个顶点(np)
        if (!mesh.is_boundary(heh)) {
            auto next_heh = mesh.next_halfedge_handle(heh);
            np = mesh.to_vertex_handle(next_heh);
        } else {
            auto opp_heh = mesh.opposite_halfedge_handle(heh);
            if (!mesh.is_boundary(opp_heh)) {
                auto next_opp_heh = mesh.next_halfedge_handle(opp_heh);
                np = mesh.to_vertex_handle(next_opp_heh);
            } else {
                continue; // 无效边界
            }
        }
        
        // 计算两个相邻三角形的面积
        float area1 = triangleArea(mesh.point(vh), mesh.point(adjV), mesh.point(pp));
        float area2 = triangleArea(mesh.point(vh), mesh.point(adjV), mesh.point(np));
        
        if (area1 > EPSILON && area2 > EPSILON) {
            // 计算余切权重
            auto vec1 = mesh.point(adjV) - mesh.point(pp);
            auto vec2 = mesh.point(vh) - mesh.point(pp);
            float cot_alpha = dot(vec1, vec2) / cross(vec1, vec2).length();
            
            auto vec3 = mesh.point(adjV) - mesh.point(np);
            auto vec4 = mesh.point(vh) - mesh.point(np);
            float cot_beta = dot(vec3, vec4) / cross(vec3, vec4).length();
            
            // 累加到平均曲率向量
            if(cot_alpha + cot_beta > 50) {
                std::cout << "dot(vec3, vec4): " << dot(vec3, vec4) << std::endl;
                std::cout << "cross(vec3, vec4): " << cross(vec3, vec4) << std::endl;
                std::cout << "vec3 (adjV - np): " << vec3 << std::endl;
                std::cout << "vec4 (vh - np): " << vec4 << std::endl;
                std::cout << "cot_alpha: " << cot_alpha << std::endl;
                std::cout << "cot_beta: " << cot_beta << std::endl;
                // std::cout << "cot_alpha + cot_beta: " << (cot_alpha + cot_beta) << std::endl;
                // std::cout << "vh point: " << mesh.point(vh) << std::endl;
                // std::cout << "adjV point: " << mesh.point(adjV) << std::endl;
                // std::cout << "point difference: " << (mesh.point(vh) - mesh.point(adjV)) << std::endl;
                // std::cout << "H before update: " << H << std::endl;
                // std::cout << "Update value: " << (cot_alpha + cot_beta) * (mesh.point(vh) - mesh.point(adjV)) << std::endl;
                // std::cout << "H after update: " << H + (cot_alpha + cot_beta) * (mesh.point(vh) - mesh.point(adjV)) << std::endl;
            }else {
                H += (cot_alpha + cot_beta) * (mesh.point(vh) - mesh.point(adjV));
            }
        }
    }
    
    return H / (4.0f * A_mixed);
}

// 辅助函数：计算三角形面积
float GLWidget::triangleArea(const Mesh::Point& p0, const Mesh::Point& p1, const Mesh::Point& p2) {
    auto e1 = p1 - p0;
    auto e2 = p2 - p0;
    auto cross = e1 % e2;
    return cross.length() / 2.0f;
}

// 计算混合面积（参考GetAmixed函数）
float GLWidget::calculateMixedArea(Mesh& mesh, const Mesh::VertexHandle& vh, const std::vector<bool>& isBoundary) {
    float A_mixed = 0.f;
    if (isBoundary[vh.idx()]) 
        return A_mixed;

    // 遍历邻接顶点（参考GetAmixed中的adjV）
    for (auto vv_it = mesh.vv_begin(vh); vv_it != mesh.vv_end(vh); ++vv_it) {
        auto adjV = *vv_it;
        
        // 找到共享边的下一个顶点np（参考GetAmixed中的np）
        Mesh::VertexHandle np;
        auto heh = mesh.find_halfedge(vh, adjV);
        if (!heh.is_valid()) continue;
        
        if (!mesh.is_boundary(heh)) {
            // 获取同一面上的下一个顶点
            auto next_heh = mesh.next_halfedge_handle(heh);
            np = mesh.to_vertex_handle(next_heh);
        } else {
            // 边界边处理：获取对边上的顶点
            auto opp_heh = mesh.opposite_halfedge_handle(heh);
            if (!mesh.is_boundary(opp_heh)) {
                auto next_opp_heh = mesh.next_halfedge_handle(opp_heh);
                np = mesh.to_vertex_handle(next_opp_heh);
            } else {
                continue; // 无效边界
            }
        }

        auto p_v = mesh.point(vh);
        auto p_adjV = mesh.point(adjV);
        auto p_np = mesh.point(np);
        
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

// 将OpenMesh网格数据更新到渲染数据结构
void GLWidget::updateMeshFromOpenMesh() {
    if (!modelLoaded || openMesh.n_vertices() == 0) {
        qWarning() << "OpenMesh is empty or model not loaded";
        return;
    }
    
    // 更新顶点数据
    vertices.clear();
    vertices.reserve(openMesh.n_vertices() * 3);
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        vertices.push_back(p[0]);
        vertices.push_back(p[1]);
        vertices.push_back(p[2]);
    }
    
    // 更新面索引数据
    faces.clear();
    faces.reserve(openMesh.n_faces() * 3);
    for (auto fh : openMesh.faces()) {
        int count = 0;
        for (auto fv_it = openMesh.fv_begin(fh); fv_it != openMesh.fv_end(fh); ++fv_it) {
            if (count++ >= 3) break; // 确保只处理三角形
            faces.push_back(fv_it->idx());
        }
    }
    
    // 重新计算边界框和缩放（可选）
    QVector3D center(0, 0, 0);
    for (size_t i = 0; i < vertices.size(); i += 3) {
        center += QVector3D(vertices[i], vertices[i+1], vertices[i+2]);
    }
    center /= openMesh.n_vertices();
    
    // 重新计算法线
    calculateNormals();
    
    // 重新计算曲率
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    vbo.bind();
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    int curvatureSize = vertexCurvatures.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize + curvatureSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    vbo.write(vertexSize + normalSize, vertexCurvatures.data(), curvatureSize);
    vbo.release();
    
    // 更新面索引缓冲区
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
    faceEbo.release();
    
    doneCurrent();
    
    // 标记模型已加载
    modelLoaded = true;
    update();
}

// 执行极小曲面迭代 (使用OpenMesh)
void GLWidget::performMinimalSurfaceIteration(int iterations, float lambda) {
    if (!modelLoaded || vertices.empty()) return;
    
    // 构建OpenMesh网格
    openMesh.clear();
    
    // 添加顶点
    for (size_t i = 0; i < vertices.size(); i += 3) {
        openMesh.add_vertex(Mesh::Point(vertices[i], vertices[i+1], vertices[i+2]));
    }
    
    // 添加面
    for (size_t i = 0; i < faces.size(); i += 3) {
        std::vector<Mesh::VertexHandle> faceVertices;
        faceVertices.push_back(Mesh::VertexHandle(faces[i]));
        faceVertices.push_back(Mesh::VertexHandle(faces[i+1]));
        faceVertices.push_back(Mesh::VertexHandle(faces[i+2]));
        openMesh.add_face(faceVertices);
    }
    
    // 请求半边属性
    openMesh.request_halfedge_status();
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        if (openMesh.is_boundary(vh)) {
            isBoundary[vh.idx()] = true;
        }
    }
    
    // 预计算所有顶点的混合面积
    std::vector<float> mixedAreas(openMesh.n_vertices(), 0.0f);
    for (auto vh : openMesh.vertices()) {
        mixedAreas[vh.idx()] = calculateMixedArea(openMesh, vh, isBoundary);
    }

    // 执行迭代
    for (int iter = 0; iter < iterations; iter++) {
        // 存储平均曲率向量
        std::vector<Mesh::Point> meanCurvatureVectors(openMesh.n_vertices(), Mesh::Point(0,0,0));
        
        // 计算每个顶点的平均曲率向量
        for (auto vh : openMesh.vertices()) {
            if (isBoundary[vh.idx()]) continue;
            
            float A_mixed = mixedAreas[vh.idx()];
            if (A_mixed < EPSILON) continue;
            
            meanCurvatureVectors[vh.idx()] = computeMeanCurvatureVector(openMesh, vh, A_mixed);
        }
        
        // 更新顶点位置
        for (auto vh : openMesh.vertices()) {
            if (isBoundary[vh.idx()]) continue;
            
            Mesh::Point H = meanCurvatureVectors[vh.idx()];
            float A = mixedAreas[vh.idx()];

            if (A > EPSILON) {
                Mesh::Point newPos = openMesh.point(vh) - lambda * H;
                openMesh.set_point(vh, newPos);
            }
        }
    }
    
    // 更新顶点位置
    for (auto vh : openMesh.vertices()) {
        auto p = openMesh.point(vh);
        vertices[vh.idx() * 3] = p[0];
        vertices[vh.idx() * 3 + 1] = p[1];
        vertices[vh.idx() * 3 + 2] = p[2];
    }
    
    // 重新计算法线和曲率
    calculateNormals();
    calculateCurvatures();
    
    // 更新OpenGL缓冲区
    makeCurrent();
    vbo.bind();
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    int curvatureSize = vertexCurvatures.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize + curvatureSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    vbo.write(vertexSize + normalSize, vertexCurvatures.data(), curvatureSize);
    vbo.release();
    doneCurrent();
    
    update();
}