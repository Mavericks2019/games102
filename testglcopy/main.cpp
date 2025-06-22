#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <vector>
#include <QDebug>
#include <QString>
#include <QtCore> 
#include <set>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyleFactory>
#include <QPalette>
#include <QGroupBox>
#include <QFormLayout>
#include <QStackedLayout>
#include <QTabWidget>
#include <QFileInfo>
#include <QSlider>
#include <QColorDialog>
#include <QSurfaceFormat>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    enum RenderMode { Wireframe, BlinnPhong };

    GLWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
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

    // 添加公共方法用于重置视图
    void resetView() {
        rotationX = rotationY = 0;
        zoom = 1.0f;
        update();
    }

    // 添加公共方法用于设置背景色
    void setBackgroundColor(const QColor& color) {
        bgColor = color;
        makeCurrent();
        glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
        doneCurrent();
        update();
    }
    
    // 设置渲染模式
    void setRenderMode(RenderMode mode) {
        currentRenderMode = mode;
        if (modelLoaded) {
            makeCurrent();
            initializeShaders();
            doneCurrent();
        }
        update();
    }

    void loadOBJ(const QString &path) {
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

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF()); // 使用成员变量背景色
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE); // 启用多重采样抗锯齿

        initializeShaders();
    }
    
    void initializeShaders() {
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

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
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

    void keyPressEvent(QKeyEvent *event) override {
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
            resetView(); // 调用公共方法
            break;
        default:
            QOpenGLWidget::keyPressEvent(event);
        }
        update();
    }

    // 鼠标事件处理：拖动旋转
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            isDragging = true;
            lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            isDragging = false;
            setCursor(Qt::ArrowCursor);
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (isDragging) {
            QPoint currentPos = event->pos();
            QPoint delta = currentPos - lastMousePos;
            
            // 根据鼠标移动距离计算旋转角度
            // X轴旋转对应垂直移动，Y轴旋转对应水平移动
            rotationY += delta.x() * 0.5f;
            rotationX += delta.y() * 0.5f;
            
            // 限制X轴旋转范围在-90到90度之间
            rotationX = qBound(-90.0f, rotationX, 90.0f);
            
            lastMousePos = currentPos;
            update();
        }
    }

    // 鼠标滚轮事件：缩放
    void wheelEvent(QWheelEvent *event) override {
        QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            float delta = numDegrees.y() > 0 ? 1.1f : 0.9f;
            zoom *= delta;
            
            // 限制缩放范围
            zoom = qBound(0.1f, zoom, 10.0f);
            
            update();
        }
    }

public:
    // 计算顶点法线
    void calculateNormals() {
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

    QOpenGLShaderProgram wireframeProgram;
    QOpenGLShaderProgram blinnPhongProgram;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ebo{QOpenGLBuffer::IndexBuffer}; // 用于线框
    QOpenGLBuffer faceEbo{QOpenGLBuffer::IndexBuffer}; // 用于面渲染

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> faces; // 面数据（每3个元素表示一个三角形）
    std::vector<unsigned int> edges; // 边数据（每2个元素表示一条边）
    std::set<uint64_t> uniqueEdges;  // 用于检测重复边

    float rotationX, rotationY;
    float zoom;
    bool modelLoaded;
    QColor bgColor; // 背景颜色
    RenderMode currentRenderMode;
    
    // 鼠标交互相关变量
    bool isDragging;
    QPoint lastMousePos;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置Fusion样式
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 设置深色主题
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(palette);
    
    // 设置字体
    QFont defaultFont("Arial", 12);
    app.setFont(defaultFont);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.resize(1920, 1080); // 设置分辨率为1920x1080
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    
    // 创建OpenGL窗口
    GLWidget *glWidget = new GLWidget;
    
    // 创建TabWidget
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(glWidget, "OBJ Model");
    mainLayout->addWidget(tabWidget, 5); // 占据大部分空间
    
    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget;
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);
    
    // 点信息组
    QGroupBox *pointGroup = new QGroupBox("Model Information");
    QVBoxLayout *pointLayout = new QVBoxLayout(pointGroup);
    
    // 声明 pointInfoLabel
    QLabel *pointInfoLabel = new QLabel("No model loaded");
    pointInfoLabel->setAlignment(Qt::AlignCenter);
    pointInfoLabel->setFixedHeight(50);
    pointInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    pointInfoLabel->setWordWrap(true);
    
    pointLayout->addWidget(pointInfoLabel);
    controlLayout->addWidget(pointGroup);
    
    // 创建堆叠布局用于切换控制面板
    QStackedLayout *stackedControlLayout = new QStackedLayout;
    controlLayout->addLayout(stackedControlLayout);
    
    // 添加OBJ控制面板
    QWidget *objControlPanel = new QWidget;
    QVBoxLayout *objControlLayout = new QVBoxLayout(objControlPanel);
    
    // 添加OBJ加载按钮
    QPushButton *loadButton = new QPushButton("Load OBJ File");
    loadButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(loadButton, &QPushButton::clicked, [glWidget, pointInfoLabel, &mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            &mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            glWidget->loadOBJ(filePath);
            // 更新模型信息
            pointInfoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
            // 更新窗口标题显示文件名
            mainWindow.setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName());
        }
    });
    objControlLayout->addWidget(loadButton);
    
    // 添加渲染模式切换按钮
    QPushButton *renderModeButton = new QPushButton("Switch to Solid Rendering");
    renderModeButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(renderModeButton, &QPushButton::clicked, [glWidget, renderModeButton]() {
        if (glWidget->currentRenderMode == GLWidget::Wireframe) {
            glWidget->setRenderMode(GLWidget::BlinnPhong);
            renderModeButton->setText("Switch to Wireframe Rendering");
        } else {
            glWidget->setRenderMode(GLWidget::Wireframe);
            renderModeButton->setText("Switch to Solid Rendering");
        }
    });
    objControlLayout->addWidget(renderModeButton);
    
    // 添加重置视图按钮
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    // 使用公共方法代替直接调用受保护的方法
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget]() {
        glWidget->resetView();
    });
    objControlLayout->addWidget(resetButton);
    
    // 添加使用说明
    QLabel *infoLabel = new QLabel(
        "<b>Instructions:</b><br>"
        "• Load an OBJ model using the button above<br>"
        "• Mouse drag: Rotate the model<br>"
        "• Mouse wheel: Zoom in/out<br>"
        "• Arrow keys: Fine-tune rotation<br>"
        "• '+'/'-' keys: Zoom in/out<br>"
        "• 'R' key: Reset view<br>"
        "• Switch between wireframe and solid rendering"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    objControlLayout->addWidget(infoLabel);
    
    // 添加光照控制组
    QGroupBox *lightGroup = new QGroupBox("Lighting Controls");
    lightGroup->setStyleSheet("QGroupBox { color: white; font-size: 14px; }");
    QFormLayout *lightLayout = new QFormLayout(lightGroup);
    
    // 环境光强度滑块
    QSlider *ambientSlider = new QSlider(Qt::Horizontal);
    ambientSlider->setRange(0, 100);
    ambientSlider->setValue(30); // 30% 环境光
    lightLayout->addRow("Ambient:", ambientSlider);
    
    // 漫反射强度滑块
    QSlider *diffuseSlider = new QSlider(Qt::Horizontal);
    diffuseSlider->setRange(0, 100);
    diffuseSlider->setValue(70); // 70% 漫反射
    lightLayout->addRow("Diffuse:", diffuseSlider);
    
    // 镜面反射强度滑块
    QSlider *specularSlider = new QSlider(Qt::Horizontal);
    specularSlider->setRange(0, 100);
    specularSlider->setValue(40); // 40% 镜面反射
    lightLayout->addRow("Specular:", specularSlider);
    
    // 高光指数滑块
    QSlider *shininessSlider = new QSlider(Qt::Horizontal);
    shininessSlider->setRange(1, 256);
    shininessSlider->setValue(32); // 默认高光指数
    lightLayout->addRow("Shininess:", shininessSlider);
    
    objControlLayout->addWidget(lightGroup);
    
    // 添加背景颜色按钮
    QPushButton *bgColorButton = new QPushButton("Change Background Color");
    bgColorButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    // 使用公共方法设置背景色
    QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget, &mainWindow]() {
        QColor color = QColorDialog::getColor(Qt::black, &mainWindow, "Select Background Color");
        if (color.isValid()) {
            glWidget->setBackgroundColor(color);
        }
    });
    objControlLayout->addWidget(bgColorButton);
    
    objControlLayout->addStretch();
    
    // 将OBJ控制面板添加到堆叠布局
    stackedControlLayout->addWidget(objControlPanel);
    
    // 将控制面板添加到主布局
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}