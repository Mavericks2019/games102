#include "glwidget.h"
#include <QFile>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QVector3D>
#include <QtMath>

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer)
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
        makeCurrent();
        initializeShaders();
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
    
    qDebug() << "Loaded OBJ file:" << path;
    qDebug() << "Vertices:" << vertexCount << "Faces:" << faceCount;
    qDebug() << "Edges:" << edges.size() / 2;
    qDebug() << "Model center:" << center;
    qDebug() << "Model size:" << maxSize;
    
    modelLoaded = true;
    
    // 更新OpenGL缓冲区
    if (vao.isCreated()) {
        makeCurrent();
        initializeShaders();
        doneCurrent();
    }
    
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
    
    // 线框着色器程序
    if (!wireframeProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 120\n"
        "attribute vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\n")) {
        qWarning() << "Vertex shader error:" << wireframeProgram.log();
    }
    
    if (!wireframeProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 120\n"
        "void main() {\n"
        "   gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n" // 白色线条
        "}\n")) {
        qWarning() << "Fragment shader error:" << wireframeProgram.log();
    }
    
    if (!wireframeProgram.link()) {
        qWarning() << "Shader link error:" << wireframeProgram.log();
    }
    
    // 布林冯着色器程序
    if (!blinnPhongProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 120\n"
        "attribute vec3 aPos;\n"
        "attribute vec3 aNormal;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform mat3 normalMatrix;\n"
        "varying vec3 FragPos;\n"
        "varying vec3 Normal;\n"
        "void main() {\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = normalMatrix * aNormal;\n"
        "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\n")) {
        qWarning() << "Blinn-Phong vertex shader error:" << blinnPhongProgram.log();
    }
    
    if (!blinnPhongProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 120\n"
        "varying vec3 FragPos;\n"
        "varying vec3 Normal;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform vec3 lightColor;\n"
        "uniform vec3 objectColor;\n"
        "void main() {\n"
        "   // 环境光\n"
        "   float ambientStrength = 0.1;\n"
        "   vec3 ambient = ambientStrength * lightColor;\n"
        "   \n"
        "   // 漫反射\n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 lightDir = normalize(lightPos - FragPos);\n"
        "   float diff = max(dot(norm, lightDir), 0.0);\n"
        "   vec3 diffuse = diff * lightColor;\n"
        "   \n"
        "   // 镜面反射 (Blinn-Phong)\n"
        "   float specularStrength = 0.5;\n"
        "   vec3 viewDir = normalize(viewPos - FragPos);\n"
        "   vec3 halfwayDir = normalize(lightDir + viewDir);\n"
        "   float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);\n"
        "   vec3 specular = specularStrength * spec * lightColor;\n"
        "   \n"
        "   vec3 result = (ambient + diffuse + specular) * objectColor;\n"
        "   gl_FragColor = vec4(result, 1.0);\n"
        "}\n")) {
        qWarning() << "Blinn-Phong fragment shader error:" << blinnPhongProgram.log();
    }
    
    if (!blinnPhongProgram.link()) {
        qWarning() << "Blinn-Phong shader link error:" << blinnPhongProgram.log();
    }

    // 创建VBO和VAO
    vao.create();
    vbo.create();
    ebo.create();

    vao.bind();
    vbo.bind();
    
    // 分配缓冲区
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    
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

    ebo.bind();
    ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int)); // 使用边数据
    vao.release();
    
    // 创建面索引缓冲区
    faceEbo.create();
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
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

    if (currentRenderMode == Wireframe) {
        // 线框模式
        wireframeProgram.bind();
        vao.bind();

        // 设置线条宽度
        glLineWidth(1.5f);

        wireframeProgram.setUniformValue("model", model);
        wireframeProgram.setUniformValue("view", view);
        wireframeProgram.setUniformValue("projection", projection);

        // 绘制模型 - 使用线框模式 (GL_LINES)
        glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);

        vao.release();
        wireframeProgram.release();
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

        // 绘制模型 - 使用面模式 (GL_TRIANGLES)
        glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

        faceEbo.release();
        vao.release();
        blinnPhongProgram.release();
    }
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