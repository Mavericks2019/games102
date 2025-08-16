#include "glbasicwidget.h"
#include <QDebug>
#include <cmath>
#include <QDateTime>
#include <QOpenGLFunctions_4_3_Core>
#include <QFileInfo>
#include <chrono>
#include <QPainter>

GLBasicWidget::GLBasicWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setMinimumSize(600, 600);
    
    QSurfaceFormat fmt;
    //fmt.setSamples(4); // 4x MSAA
    fmt.setVersion(4, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(fmt);
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { update(); });
    timer->start(16); // ~60 FPS
    
    // 使用高精度时钟
    startTime = std::chrono::high_resolution_clock::now();
}


GLBasicWidget::~GLBasicWidget() {
    makeCurrent();
    delete program;
    vao.destroy();
    vbo.destroy();
    doneCurrent();
}

// 重置时间
void GLBasicWidget::resetTime() {
    startTime = std::chrono::high_resolution_clock::now();
    update();
}

// 重新加载着色器
void GLBasicWidget::reloadShaders() {
    makeCurrent();
    
    if (program) {
        program->release();
        delete program;
        program = nullptr;
    }
    
    program = new QOpenGLShaderProgram(this);
    
    QString vertPath = ":/shaderwidget/shaders/basic.vert";
    QString fragPath = ":/shaderwidget/shaders/basic.frag";
    
    if (!loadShader(QOpenGLShader::Vertex, vertPath)) {
        qCritical() << "Vertex shader error:" << program->log();
    }
    if (!loadShader(QOpenGLShader::Fragment, fragPath)) {
        qCritical() << "Fragment shader error:" << program->log();
    }
    if (!program->link()) {
        qCritical() << "Shader link error:" << program->log();
    }
    
    // 配置顶点属性
    if (program->isLinked()) {
        program->bind();
        
        // 位置属性 (location = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        
        // 纹理坐标属性 (location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        program->release();
    }
    
    doneCurrent();
    update();
}

void GLBasicWidget::initializeGL() {
    initializeOpenGLFunctions();
    
    // 添加上下文有效性检查
    if (!context()->isValid()) {
        qCritical() << "OpenGL context is invalid!";
        return;
    }
    
    // 打印OpenGL版本信息
    qDebug() << "OpenGL Version:" << QString::fromLatin1((const char*)glGetString(GL_VERSION));
    qDebug() << "GLSL Version:" << QString::fromLatin1((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // 创建着色器程序
    program = new QOpenGLShaderProgram(this);
    
    // 检查着色器文件是否存在
    QString vertPath = ":/shaderwidget/shaders/basic.vert";
    QString fragPath = ":/shaderwidget/shaders/basic.frag";
    
    qDebug() << "Vertex shader path:" << QFileInfo(vertPath).absoluteFilePath();
    qDebug() << "Fragment shader path:" << QFileInfo(fragPath).absoluteFilePath();
    
    if (!QFile::exists(vertPath)) {
        qCritical() << "Vertex shader file not found:" << vertPath;
    }
    if (!QFile::exists(fragPath)) {
        qCritical() << "Fragment shader file not found:" << fragPath;
    }
    
    // 加载着色器
    if (!loadShader(QOpenGLShader::Vertex, vertPath)) {
        qCritical() << "Vertex shader error:" << program->log();
    }
    if (!loadShader(QOpenGLShader::Fragment, fragPath)) {
        qCritical() << "Fragment shader error:" << program->log();
    }
    if (!program->link()) {
        qCritical() << "Shader link error:" << program->log();
    }
    
    // 检查着色器是否成功链接
    if (!program->isLinked()) {
        qCritical() << "Shader program failed to link!";
        return;
    }
    
    // 创建VAO和VBO
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    
    // 全屏矩形顶点数据 (位置 + 纹理坐标)
    const float vertices[] = {
        // 位置          // 纹理坐标
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    vbo.allocate(vertices, sizeof(vertices));
    
    // 配置顶点属性
    program->bind();
    
    // 位置属性 (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // 纹理坐标属性 (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    program->release();
    vbo.release();
    vao.release();
    
    // 检查OpenGL错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qDebug() << "OpenGL error after initialization:" << err;
    }
    
    qDebug() << "OpenGL initialization complete";
}

void GLBasicWidget::paintGL() {
        // === 帧率计算开始 ===
    static QElapsedTimer fpsTimer;
    static int frameCount = 0;
    static float fps = 0.0f;
    
    if (frameCount == 0) {
        fpsTimer.start();
    }
    frameCount++;
    
    // 每0.5秒更新一次帧率
    if (fpsTimer.elapsed() > 500) {
        fps = frameCount * 1000.0f / fpsTimer.elapsed();
        frameCount = 0;
        fpsTimer.restart();
    }
    // === 帧率计算结束 ===
    // 清除缓冲区
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!program || !program->isLinked()) {
        qDebug() << "Shader program not ready for painting";
        return;
    }
    
    program->bind();
    vao.bind();
    
    // 使用高精度时间计算 - 修复精度问题
    auto now = std::chrono::high_resolution_clock::now();
    float elapsedTime = std::chrono::duration<float>(now - startTime).count();
    
    // 设置统一变量
    program->setUniformValue("iTime", elapsedTime);
    program->setUniformValue("iResolution", QVector2D(width(), height()));
    
    // 绘制全屏矩形
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    vao.release();
    program->release();
    
    // 检查OpenGL错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qDebug() << "OpenGL error after paintGL:" << err;
    }
    // === 在右下角绘制帧率 ===
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    
    // 创建半透明背景
    QRect textRect(width() - 120, height() - 40, 110, 30);
    painter.fillRect(textRect, QColor(0, 0, 0, 150));
    
    // 绘制帧率文本
    QString fpsText = QString("FPS: %1").arg(fps, 0, 'f', 1);
    painter.drawText(textRect, Qt::AlignCenter, fpsText);
    // === 帧率绘制结束 ===
}

void GLBasicWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    currentWidth = w;
    currentHeight = h;
    emit resolutionChanged(w, h);
    qDebug() << "Resized to:" << w << "x" << h;
}

bool GLBasicWidget::loadShader(QOpenGLShader::ShaderType type, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open shader file:" << filePath;
        return false;
    }
    
    QTextStream stream(&file);
    QString shaderSource = stream.readAll();
    file.close();
    
    // 添加调试信息
    qDebug() << "Loading shader:" << QFileInfo(filePath).absoluteFilePath();
    if (shaderSource.isEmpty()) {
        qWarning() << "Shader source is empty for file:" << filePath;
        return false;
    }
    
    bool success = program->addShaderFromSourceCode(type, shaderSource);
    if (!success) {
        qDebug() << "Shader compilation error (" << filePath << "):" << program->log();
    }
    return success;
}