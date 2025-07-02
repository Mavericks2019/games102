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

#define EPSILON 1E-4F 

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
    if (m_useHalfEdgeForCurvature) {
        calculateCurvaturesHemesh();
    } else {
        calculateCurvaturesAdjacency();
    }
}

// 添加新的曲率计算函数
void GLWidget::calculateCurvaturesHemesh() {
    gaussianCurvatures.clear();
    meanCurvatures.clear();
    maxCurvatures.clear();
    
    if (vertices.empty() || faces.empty()) return;
    
    // 构建半边数据结构
    m_hemesh.build(vertices, faces);
    m_hemesh.calculateCurvatures();
    
    // 获取曲率数据
    size_t vertexCount = vertices.size() / 3;
    gaussianCurvatures.resize(vertexCount);
    meanCurvatures.resize(vertexCount);
    maxCurvatures.resize(vertexCount);
    
    const auto& hemVertices = m_hemesh.getVertices();
    for (size_t i = 0; i < vertexCount; i++) {
        gaussianCurvatures[i] = hemVertices[i]->gaussianCurvature;
        meanCurvatures[i] = hemVertices[i]->meanCurvature;
        maxCurvatures[i] = hemVertices[i]->maxCurvature;
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
}

void GLWidget::calculateCurvaturesAdjacency()
{
    gaussianCurvatures.clear();
    meanCurvatures.clear();
    maxCurvatures.clear();
    
    if (vertices.empty() || faces.empty()) return;
    
    // 构建邻接图
    adjacencyGraph.build(vertices, faces);
    const auto& adjacency = adjacencyGraph.getAdjacency();
    
    // 自定义余切计算函数
    auto cotangent = [](const QVector3D& v1, const QVector3D& v2) -> float {
        float dot = QVector3D::dotProduct(v1, v2);
        QVector3D cross = QVector3D::crossProduct(v1, v2);
        float crossLength = cross.length();
        return dot / (crossLength + 1e-6f); // 避免除以零
    };
    
    // 自定义角度计算函数
    auto angle = [](const QVector3D& v1, const QVector3D& v2) -> float {
        float dot = QVector3D::dotProduct(v1, v2);
        float len1 = v1.length();
        float len2 = v2.length();
        
        if (len1 < 1e-6f || len2 < 1e-6f) return 0.0f;
        
        float cosTheta = dot / (len1 * len2);
        cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
        return std::acos(cosTheta);
    };
    
    // 初始化曲率存储
    size_t vertexCount = vertices.size() / 3;
    gaussianCurvatures.resize(vertexCount, 0.0f);
    meanCurvatures.resize(vertexCount, 0.0f);
    maxCurvatures.resize(vertexCount, 0.0f);
    
    // 计算每个顶点的混合面积 (类似GetAmixed函数)
    std::vector<float> A_mixed(vertexCount, 0.0f);
    
    // 首先计算每个三角形的面积
    std::vector<float> triangleAreas(faces.size() / 3, 0.0f);
    for (size_t i = 0; i < faces.size(); i += 3) {
        unsigned int idx1 = faces[i];
        unsigned int idx2 = faces[i+1];
        unsigned int idx3 = faces[i+2];
        
        QVector3D v1(vertices[idx1*3], vertices[idx1*3+1], vertices[idx1*3+2]);
        QVector3D v2(vertices[idx2*3], vertices[idx2*3+1], vertices[idx2*3+2]);
        QVector3D v3(vertices[idx3*3], vertices[idx3*3+1], vertices[idx3*3+2]);
        
        QVector3D edge1 = v2 - v1;
        QVector3D edge2 = v3 - v1;
        float area = QVector3D::crossProduct(edge1, edge2).length() / 2.0f;
        triangleAreas[i/3] = area;
    }
    
    // 使用邻接图计算混合面积
    for (size_t i = 0; i < vertexCount; i++) {
        if (adjacency[i].adjacentFaces.empty()) continue;
        
        for (size_t faceIdx : (adjacency[i].adjacentFaces)) {
            size_t faceStart = faceIdx * 3;
            unsigned int idx1 = faces[faceStart];
            unsigned int idx2 = faces[faceStart+1];
            unsigned int idx3 = faces[faceStart+2];
            
            QVector3D v1(vertices[idx1*3], vertices[idx1*3+1], vertices[idx1*3+2]);
            QVector3D v2(vertices[idx2*3], vertices[idx2*3+1], vertices[idx2*3+2]);
            QVector3D v3(vertices[idx3*3], vertices[idx3*3+1], vertices[idx3*3+2]);
            
            float area = triangleAreas[faceIdx];
            
            if (area < EPSILON) continue;
            
            // 检查是否为钝角三角形
            float dot1 = QVector3D::dotProduct(v2 - v1, v3 - v1);
            float dot2 = QVector3D::dotProduct(v1 - v2, v3 - v2);
            float dot3 = QVector3D::dotProduct(v1 - v3, v2 - v3);
            
            // 非钝角三角形
            if (dot1 >= 0 && dot2 >= 0 && dot3 >= 0) {
                // 计算余切权重
                float cotA = cotangent(v2 - v1, v3 - v1);
                float cotB = cotangent(v3 - v2, v1 - v2);
                float cotC = cotangent(v1 - v3, v2 - v3);
                
                // 更新顶点混合面积
                if (i == idx1) {
                    A_mixed[i] += (cotB + cotC) * (v2 - v1).lengthSquared() / 8.0f;
                } else if (i == idx2) {
                    A_mixed[i] += (cotA + cotC) * (v3 - v2).lengthSquared() / 8.0f;
                } else if (i == idx3) {
                    A_mixed[i] += (cotA + cotB) * (v1 - v3).lengthSquared() / 8.0f;
                }
            } else {
                // 钝角三角形
                if (dot1 < 0) {
                    // 在v1处为钝角
                    if (i == idx1) A_mixed[i] += area / 2.0f;
                    else if (i == idx2 || i == idx3) A_mixed[i] += area / 4.0f;
                } else if (dot2 < 0) {
                    // 在v2处为钝角
                    if (i == idx2) A_mixed[i] += area / 2.0f;
                    else if (i == idx1 || i == idx3) A_mixed[i] += area / 4.0f;
                } else {
                    // 在v3处为钝角
                    if (i == idx3) A_mixed[i] += area / 2.0f;
                    else if (i == idx1 || i == idx2) A_mixed[i] += area / 4.0f;
                }
            }
        }
    }
    
    // 计算高斯曲率和平均曲率
    for (size_t i = 0; i < vertexCount; i++) {
        if (A_mixed[i] < EPSILON) continue;
        
        // 使用邻接图计算高斯曲率
        float angleSum = 0.0f;
        for (size_t faceIdx : (adjacency[i].adjacentFaces)) {
            size_t faceStart = faceIdx * 3;
            unsigned int idx1 = faces[faceStart];
            unsigned int idx2 = faces[faceStart+1];
            unsigned int idx3 = faces[faceStart+2];
            
            QVector3D v1(vertices[idx1*3], vertices[idx1*3+1], vertices[idx1*3+2]);
            QVector3D v2(vertices[idx2*3], vertices[idx2*3+1], vertices[idx2*3+2]);
            QVector3D v3(vertices[idx3*3], vertices[idx3*3+1], vertices[idx3*3+2]);
            
            // 计算角度
            if (i == idx1) {
                QVector3D vec1 = v2 - v1;
                QVector3D vec2 = v3 - v1;
                angleSum += angle(vec1, vec2);
            } else if (i == idx2) {
                QVector3D vec1 = v1 - v2;
                QVector3D vec2 = v3 - v2;
                angleSum += angle(vec1, vec2);
            } else if (i == idx3) {
                QVector3D vec1 = v1 - v3;
                QVector3D vec2 = v2 - v3;
                angleSum += angle(vec1, vec2);
            }
        }
        
        gaussianCurvatures[i] = (2 * M_PI - angleSum) / A_mixed[i];
        
        // 使用邻接图计算平均曲率
        QVector3D meanCurvature(0, 0, 0);
        for (size_t j = 0; j < adjacency[i].neighbors.size(); j++) {
            size_t neighborIdx = adjacency[i].neighbors[j];
            QVector3D vi(vertices[i*3], vertices[i*3+1], vertices[i*3+2]);
            QVector3D vj(vertices[neighborIdx*3], vertices[neighborIdx*3+1], vertices[neighborIdx*3+2]);
            
            // 查找共享边
            for (size_t faceIdx : adjacency[i].adjacentFaces) {
                size_t faceStart = faceIdx * 3;
                unsigned int idx1 = faces[faceStart];
                unsigned int idx2 = faces[faceStart+1];
                unsigned int idx3 = faces[faceStart+2];
                
                // 检查是否包含邻居顶点
                if (neighborIdx != idx1 && neighborIdx != idx2 && neighborIdx != idx3) continue;
                
                // 找到对边顶点
                unsigned int oppIdx = idx1;
                if (i == idx1) {
                    if (neighborIdx == idx2) oppIdx = idx3;
                    else oppIdx = idx2;
                } else if (i == idx2) {
                    if (neighborIdx == idx1) oppIdx = idx3;
                    else oppIdx = idx1;
                } else {
                    if (neighborIdx == idx1) oppIdx = idx2;
                    else oppIdx = idx1;
                }
                
                QVector3D v_opp(vertices[oppIdx*3], vertices[oppIdx*3+1], vertices[oppIdx*3+2]);
                
                // 计算余切权重
                QVector3D edge1 = v_opp - vi;
                QVector3D edge2 = v_opp - vj;
                
                float cotWeight = cotangent(edge1, edge2);
                meanCurvature += cotWeight * (vj - vi);
                break;
            }
        }
        
        meanCurvatures[i] = meanCurvature.length() / (2.0f * A_mixed[i]);
    }
    
    // 计算最大曲率
    for (size_t i = 0; i < vertexCount; i++) {
        maxCurvatures[i] = gaussianCurvatures[i] + meanCurvatures[i];
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
    
    normalize(gaussianCurvatures);
    normalize(meanCurvatures);
    normalize(maxCurvatures);
    
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

// 执行极小曲面迭代
void GLWidget::performMinimalSurfaceIteration(int iterations, float lambda) {
    if (!modelLoaded || vertices.empty()) return;
    
    // 确保半边网格已经构建
    if (m_hemesh.getVertices().empty()) {
        m_hemesh.build(vertices, faces);
    }
    
    // 计算每个顶点的混合面积
    std::vector<float> mixedAreas(m_hemesh.getVertices().size(), 0.0f);
    for (size_t i = 0; i < m_hemesh.getVertices().size(); i++) {
        mixedAreas[i] = m_hemesh.calculateMixedArea(m_hemesh.getVertices()[i]);
    }
    
    for (int iter = 0; iter < iterations; iter++) {
        // 计算每个顶点的平均曲率向量
        std::vector<QVector3D> meanCurvatureVectors(m_hemesh.getVertices().size(), QVector3D(0,0,0));
        
        for (size_t i = 0; i < m_hemesh.getVertices().size(); i++) {
            HVertex* vertex = m_hemesh.getVertices()[i];
            if (m_hemesh.isBoundaryVertex(vertex)) continue; // 跳过边界顶点
            
            QVector3D meanCurvatureVec(0,0,0);
            HEdge* startEdge = vertex->edge;
            if (!startEdge) continue;
            
            HEdge* edge = startEdge;
            do {
                if (!edge || !edge->face) {
                    if (edge && edge->twin) edge = edge->twin->next;
                    else edge = nullptr;
                    continue;
                }
                
                HEdge* e0 = edge;
                HEdge* e1 = e0->next;
                if (!e1) {
                    edge = (edge->twin) ? edge->twin->next : nullptr;
                    continue;
                }
                
                HEdge* e2 = e1->next;
                if (!e2) {
                    edge = (edge->twin) ? edge->twin->next : nullptr;
                    continue;
                }
                
                if (!e0->vertex || !e1->vertex || !e2->vertex) {
                    edge = (edge->twin) ? edge->twin->next : nullptr;
                    continue;
                }
                
                // 获取三角形顶点
                QVector3D v0 = e0->vertex->position;
                QVector3D v1 = e1->vertex->position;
                QVector3D v2 = e2->vertex->position;
                
                // 计算余切权重
                QVector3D edgeVec = v1 - v0;
                QVector3D oppVec = v2 - v0;
                float cotWeight = m_hemesh.cotangent(edgeVec, oppVec);
                
                // 累加到平均曲率向量
                meanCurvatureVec += cotWeight * (v1 - v0);
                
                // 移动到下一个邻接半边
                edge = (edge->twin) ? edge->twin->next : nullptr;
            } while (edge && edge != startEdge);
            
            meanCurvatureVectors[i] = meanCurvatureVec;
        }
        
        // 更新顶点位置
        for (size_t i = 0; i < m_hemesh.getVertices().size(); i++) {
            HVertex* vertex = m_hemesh.getVertices()[i];
            if (m_hemesh.isBoundaryVertex(vertex)) continue; // 边界顶点不动
            
            // 计算法线方向（使用平均曲率向量方向）
            QVector3D H = meanCurvatureVectors[i];
            float H_length = H.length();
            if (H_length < EPSILON) continue;
            
            QVector3D n = H.normalized();
            
            // 更新顶点位置：v_new = v_old - λ * (H / (2A))
            // 其中 H 是平均曲率向量，A 是混合面积
            float A = mixedAreas[i];
            if (A > EPSILON) {
                vertex->position -= lambda * (H / (2.0f * A));
            }
        }
    }
    
    // 更新vertices数组
    for (size_t i = 0; i < m_hemesh.getVertices().size(); i++) {
        vertices[i*3]   = m_hemesh.getVertices()[i]->position.x();
        vertices[i*3+1] = m_hemesh.getVertices()[i]->position.y();
        vertices[i*3+2] = m_hemesh.getVertices()[i]->position.z();
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