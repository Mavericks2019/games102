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

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    GLWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        rotationX = rotationY = 0;
        zoom = 1.0f;
        modelLoaded = false;
        isDragging = false;
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
        
        qDebug() << "Loaded OBJ file:" << path;
        qDebug() << "Vertices:" << vertexCount << "Faces:" << faceCount;
        qDebug() << "Edges:" << edges.size() / 2;
        qDebug() << "Model center:" << center;
        qDebug() << "Model size:" << maxSize;
        
        modelLoaded = true;
        
        // 更新OpenGL缓冲区
        if (vao.isCreated()) {
            makeCurrent();
            vbo.bind();
            vbo.allocate(vertices.data(), vertices.size() * sizeof(float));
            ebo.bind();
            ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int)); // 使用边数据
            doneCurrent();
        }
        
        // 重置视图参数
        rotationX = rotationY = 0;
        zoom = 1.0f;
        
        update(); // 触发重绘
    }

public:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // 恢复原始背景色 (深绿色)
        glEnable(GL_DEPTH_TEST);

        // 创建着色器程序
        if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex,
            "#version 120\n"
            "attribute vec3 aPos;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
            "}\n")) {
            qWarning() << "Vertex shader error:" << program.log();
        }
        
        // 恢复原始线条颜色 (黄色)
        if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment,
            "#version 120\n"
            "void main() {\n"
            "   gl_FragColor = vec4(0.8, 0.8, 0.2, 1.0);\n" // 黄色线条
            "}\n")) {
            qWarning() << "Fragment shader error:" << program.log();
        }
        
        if (!program.link()) {
            qWarning() << "Shader link error:" << program.log();
        }

        // 创建VBO和VAO
        vao.create();
        vbo.create();
        ebo.create();

        vao.bind();
        vbo.bind();
        
        int posLoc = program.attributeLocation("aPos");
        if (posLoc != -1) {
            program.enableAttributeArray(posLoc);
            program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
        } else {
            qWarning() << "Failed to find attribute location for aPos";
        }

        ebo.bind();
        vao.release();
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!modelLoaded || vertices.empty() || edges.empty()) {
            return;
        }

        program.bind();
        vao.bind();

        // 设置线条宽度
        glLineWidth(1.5f); // 增加线条宽度提高可见性

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

        program.setUniformValue("model", model);
        program.setUniformValue("view", view);
        program.setUniformValue("projection", projection);

        // 绘制模型 - 使用线框模式 (GL_LINES)
        glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);

        vao.release();
        program.release();
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
            rotationX = rotationY = 0;
            zoom = 1.0f;
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

private:
    QOpenGLShaderProgram program;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ebo{QOpenGLBuffer::IndexBuffer};

    std::vector<float> vertices;
    std::vector<unsigned int> faces; // 保留面数据用于可能的未来扩展
    std::vector<unsigned int> edges; // 边数据（每2个元素表示一条边）
    std::set<uint64_t> uniqueEdges;  // 用于检测重复边

    float rotationX, rotationY;
    float zoom;
    bool modelLoaded;
    
    // 鼠标交互相关变量
    bool isDragging;
    QPoint lastMousePos;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWindow;
    mainWindow.resize(1920, 1080); // 设置为1920x1080分辨率
    
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(&mainWindow);
    
    // 创建标题
    QLabel *titleLabel = new QLabel("OBJ Viewer");
    titleLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #333; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // 创建OpenGL窗口
    GLWidget *glWidget = new GLWidget;
    mainLayout->addWidget(glWidget, 1); // 占据大部分空间
    
    // 创建控制面板
    QWidget *controlPanel = new QWidget;
    QHBoxLayout *controlLayout = new QHBoxLayout(controlPanel);
    
    // 添加文件按钮
    QPushButton *openButton = new QPushButton("Open OBJ File");
    openButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
    );
    
    // 添加重置按钮
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #0b7dda; }"
    );
    
    // 添加帮助文本
    QLabel *helpLabel = new QLabel("Controls: Left drag to rotate, Scroll to zoom, Arrow keys to adjust");
    helpLabel->setStyleSheet("QLabel { color: #666; font-size: 14px; }");
    
    // 布局控制面板
    controlLayout->addWidget(openButton);
    controlLayout->addWidget(resetButton);
    controlLayout->addStretch();
    controlLayout->addWidget(helpLabel);
    
    mainLayout->addWidget(controlPanel);
    
    // 连接按钮信号
    QObject::connect(openButton, &QPushButton::clicked, [&]() {
        QString filePath = QFileDialog::getOpenFileName(
            &mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            glWidget->loadOBJ(filePath);
            // 更新窗口标题显示文件名
            QFileInfo fileInfo(filePath);
            mainWindow.setWindowTitle("OBJ Viewer - " + fileInfo.fileName());
        }
    });
    
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget]() {
        glWidget->keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier));
    });

    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}