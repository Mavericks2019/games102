#include "renderwidget.h"
#include <QFile>
#include <QDebug>
#include <QOpenGLExtraFunctions> // Replace QOpenGLFunctions with this

RenderWidget::RenderWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
    timer.start(16, this); // ~60 FPS
}

RenderWidget::~RenderWidget() {
    makeCurrent();
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (textureID) glDeleteTextures(1, &textureID);
    doneCurrent();
}

void RenderWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    // 创建输出纹理
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width(), height(), 0, GL_RGBA, GL_FLOAT, nullptr);
    
    initShaders();
    createQuad();
    initScene();
}

void RenderWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    aspectRatio = static_cast<float>(w) / h;
    
    // 调整纹理大小
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    
    // 重置帧计数
    frameCount = 0;
}

void RenderWidget::paintGL() {
    // 使用计算着色器进行路径追踪
    computeShader.bind();
    computeShader.setUniformValue("frame", frameCount++);
    computeShader.setUniformValue("aspectRatio", aspectRatio);
    
    glBindImageTexture(0, textureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute((width() + 7) / 8, (height() + 7) / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    // 渲染全屏四边形
    glClear(GL_COLOR_BUFFER_BIT);
    quadShader.bind();
    glBindTexture(GL_TEXTURE_2D, textureID);
    renderQuad();
}

void RenderWidget::timerEvent(QTimerEvent *e) {
    if (e->timerId() == timer.timerId()) {
        update();
    }
}

void RenderWidget::initShaders() {
    // 计算着色器
    computeShader.addShaderFromSourceFile(QOpenGLShader::Compute, "shaders/compute.comp");
    computeShader.link();
    
    // 四边形着色器
    quadShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/quad.vert");
    quadShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/quad.frag");
    quadShader.link();
}

void RenderWidget::createQuad() {
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void RenderWidget::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void RenderWidget::initScene() {
    // 场景数据可以在这里初始化
    // 实际实现中会使用UBO或SSBO传递场景数据
}