#include "cvt_imageglwidget.h"
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLShaderProgram>
#include <QDebug>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>

CVTImageGLWidget::CVTImageGLWidget(QWidget *parent) : 
    QOpenGLWidget(parent),
    pointVbo(QOpenGLBuffer::VertexBuffer)
{
    QSurfaceFormat format;
    format.setSamples(4); // 4x MSAA
    setFormat(format);
    setFocusPolicy(Qt::StrongFocus);
}

CVTImageGLWidget::~CVTImageGLWidget()
{
    makeCurrent();
    pointVao.destroy();
    pointVbo.destroy();
    
    // 释放纹理资源
    if (imageTexture) {
        delete imageTexture;
        imageTexture = nullptr;
    }

    doneCurrent();
}

void CVTImageGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // 启用多重采样抗锯齿

    // 初始化点绘制的VAO和VBO
    pointVao.create();
    pointVbo.create();

    initializeShaders();
    initializeImageShaders();
}

void CVTImageGLWidget::initializeShaders()
{
    // 点绘制着色器 - 从文件加载
    if (!pointProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/cvtwidget/shaders/cvt_point.vert")) {
        qWarning() << "Point vertex shader error:" << pointProgram.log();
    }
    
    if (!pointProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/cvtwidget/shaders/cvt_point.frag")) {
        qWarning() << "Point fragment shader error:" << pointProgram.log();
    }
    
    if (!pointProgram.link()) {
        qWarning() << "Point shader link error:" << pointProgram.log();
    }
}

void CVTImageGLWidget::initializeImageShaders()
{
    // 图像绘制着色器
    if (!imageProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/cvtwidget/shaders/image.vert")) {
        qWarning() << "Image vertex shader error:" << imageProgram.log();
    }
    
    if (!imageProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/cvtwidget/shaders/image.frag")) {
        qWarning() << "Image fragment shader error:" << imageProgram.log();
    }
    
    if (!imageProgram.link()) {
        qWarning() << "Image shader link error:" << imageProgram.log();
    }
}

void CVTImageGLWidget::loadImage(const QImage& image)
{
    makeCurrent();
    
    // 删除现有纹理
    if (imageTexture) {
        delete imageTexture;
        imageTexture = nullptr;
    }
    
    // 保存图像 - 垂直翻转以匹配OpenGL坐标系
    loadedImage = image.convertToFormat(QImage::Format_RGBA8888)
                 .mirrored(false, true); // false: 水平不翻转, true: 垂直翻转
    
    // 创建纹理
    if (!loadedImage.isNull()) {
        imageTexture = new QOpenGLTexture(loadedImage);
        imageTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        imageTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    }
    
    doneCurrent();
    update();
}

void CVTImageGLWidget::setShowImage(bool show)
{
    showImage = show;
    update();
}

void CVTImageGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 如果有加载图像但不显示图像，绘制白色背景（保持原图比例）
    if (!loadedImage.isNull() && !showImage) {
        drawWhiteImageBackground();
    } 
    // 显示图像
    else if (!loadedImage.isNull() && showImage) {
        drawImage();
    }
    // 如果没有图像，绘制默认白色背景
    else {
        drawBackground();
    }
    
    // 然后绘制CVT内容（Voronoi图、Delaunay三角剖分、点）
    drawCVTContent();
}

void CVTImageGLWidget::drawImage()
{
    if (!imageTexture || !showImage || loadedImage.isNull()) {
        return;
    }
    
    // 设置混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 获取窗口尺寸
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;
    
    // 设置正交投影
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    
    // 计算图像比例
    float imgAspect = static_cast<float>(loadedImage.width()) / loadedImage.height();
    float drawWidth, drawHeight;
    
    if (imgAspect > aspect) {
        // 宽度充满
        drawWidth = 2.0f * aspect;
        drawHeight = drawWidth / imgAspect;
    } else {
        // 高度充满
        drawHeight = 2.0f;
        drawWidth = drawHeight * imgAspect;
    }
    
    // 顶点数据（包含纹理坐标）
    float vertices[] = {
        // 位置              // 纹理坐标
        -drawWidth/2, -drawHeight/2, 0.0f,  0.0f, 0.0f,  // 左下
         drawWidth/2, -drawHeight/2, 0.0f,  1.0f, 0.0f,  // 右下
         drawWidth/2,  drawHeight/2, 0.0f,  1.0f, 1.0f,  // 右上
        -drawWidth/2,  drawHeight/2, 0.0f,  0.0f, 1.0f   // 左上
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // 绑定着色器
    imageProgram.bind();
    imageTexture->bind(0);
    imageProgram.setUniformValue("textureSampler", 0);
    imageProgram.setUniformValue("projection", projection);
    
    // 创建临时VAO/VBO
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer);
    
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));
    
    ebo.create();
    ebo.bind();
    ebo.allocate(indices, sizeof(indices));
    
    // 设置顶点属性
    int posLoc = imageProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        imageProgram.enableAttributeArray(posLoc);
        imageProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 5 * sizeof(float));
    }
    
    int texLoc = imageProgram.attributeLocation("aTexCoord");
    if (texLoc != -1) {
        imageProgram.enableAttributeArray(texLoc);
        imageProgram.setAttributeBuffer(texLoc, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    }
    
    // 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // 清理
    vao.release();
    vbo.release();
    ebo.release();
    imageTexture->release();
    imageProgram.release();
    
    glDisable(GL_BLEND);
}

void CVTImageGLWidget::drawWhiteImageBackground()
{
    // 获取窗口尺寸
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;
    
    // 设置正交投影
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    
    // 计算图像比例
    float imgAspect = static_cast<float>(loadedImage.width()) / loadedImage.height();
    float drawWidth, drawHeight;
    
    if (imgAspect > aspect) {
        // 宽度充满
        drawWidth = 2.0f * aspect;
        drawHeight = drawWidth / imgAspect;
    } else {
        // 高度充满
        drawHeight = 2.0f;
        drawWidth = drawHeight * imgAspect;
    }
    
    // 顶点数据
    float vertices[] = {
        // 位置              
        -drawWidth/2, -drawHeight/2, 0.0f,
         drawWidth/2, -drawHeight/2, 0.0f,
         drawWidth/2,  drawHeight/2, 0.0f,
        -drawWidth/2,  drawHeight/2, 0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // 使用着色器程序
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * vec4(aPos, 1.0);\n"
        "}\n"
    );
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0); // 白色\n"
        "}\n"
    );
    if (!program.link()) {
        qWarning() << "Shader link error in drawWhiteImageBackground:" << program.log();
        return;
    }
    
    program.bind();
    program.setUniformValue("projection", projection);
    
    // 创建临时VAO/VBO
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer);
    
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));
    
    ebo.create();
    ebo.bind();
    ebo.allocate(indices, sizeof(indices));
    
    // 设置顶点属性
    int posLoc = program.attributeLocation("aPos");
    if (posLoc != -1) {
        program.enableAttributeArray(posLoc);
        program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    // 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // 清理
    vao.release();
    vbo.release();
    ebo.release();
    program.release();
}

void CVTImageGLWidget::drawBackground()
{
    // 获取窗口尺寸
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;
    
    // 设置正交投影
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    
    // 顶点数据
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // 使用着色器程序
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * vec4(aPos, 1.0);\n"
        "}\n"
    );
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0); // 白色\n"
        "}\n"
    );
    if (!program.link()) {
        qWarning() << "Background shader link error:" << program.log();
        return;
    }
    
    program.bind();
    program.setUniformValue("projection", projection);
    
    // 创建临时VAO和VBO
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer);
    
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));
    
    ebo.create();
    ebo.bind();
    ebo.allocate(indices, sizeof(indices));
    
    // 设置顶点属性
    int posLoc = program.attributeLocation("aPos");
    if (posLoc != -1) {
        program.enableAttributeArray(posLoc);
        program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    // 绘制正方形
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // 清理
    vao.release();
    vbo.release();
    ebo.release();
    program.release();
}

void CVTImageGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void CVTImageGLWidget::drawCVTContent()
{
    // 获取窗口尺寸和宽高比
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;

    // 设置正交投影
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    
    // 创建模型矩阵
    QMatrix4x4 model;
    
    // 关闭深度测试，确保CVT内容在最上层
    glDisable(GL_DEPTH_TEST);

    if(showVoronoiDiagram) {
        drawVoronoiDiagram();
    }

    if(showDelaunay) {
        drawDelaunayTriangles();
    }

    if (!canvasData.points.empty() && showPoints) {
        drawRandomPoints();    
    }

    glEnable(GL_DEPTH_TEST);
}

// cvt_imageglwidget.cpp
void CVTImageGLWidget::generateRandomPoints(int count)
{
    canvasData.points.clear();
    
    // 获取当前边界（如果有图像则使用图像边界）
    QRectF bounds = getImageBounds();
    float left = bounds.left();
    float right = bounds.right();
    float bottom = bounds.top();    // 注意：OpenGL坐标系中y轴向上
    float top = bounds.bottom();    // 所以top > bottom
    
    // 添加四个角点（使用边界范围）
    canvasData.points.push_back(Point(left, bottom));  // 左下
    canvasData.points.push_back(Point(right, bottom)); // 右下
    canvasData.points.push_back(Point(left, top));     // 左上
    canvasData.points.push_back(Point(right, top));    // 右上

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < count; i++) {
        // 在边界范围内生成随机点
        double x = left + (static_cast<double>(std::rand()) / RAND_MAX * (right - left));
        double y = bottom + (static_cast<double>(std::rand()) / RAND_MAX * (top - bottom));
        canvasData.points.push_back(Point(x, y));
    }
    
    canvasData.dt = Delaunay(canvasData.points.begin(), canvasData.points.end());
    currentPointCount = count + 4;
    
    // 准备点数据
    std::vector<float> points;
    points.reserve(canvasData.points.size() * 2);
    for (const auto& point : canvasData.points) {
        points.push_back(static_cast<float>(point.x()));
        points.push_back(static_cast<float>(point.y()));
    }
    
    // 更新点缓冲区
    makeCurrent();
    pointVao.bind();
    pointVbo.bind();
    pointVbo.allocate(points.data(), static_cast<int>(points.size() * sizeof(float)));
    
    // 设置顶点属性
    pointProgram.bind();
    int posLoc = pointProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        pointProgram.enableAttributeArray(posLoc);
        pointProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }
    
    pointVao.release();
    pointVbo.release();
    pointProgram.release();
    
    // 生成Voronoi图数据
    computeVoronoiDiagram();
    
    doneCurrent();
    update();
}

void CVTImageGLWidget::setShowPoints(bool show)
{
    showPoints = show;
    update();
}

void CVTImageGLWidget::setShowVoronoiDiagram(bool show)
{
    showVoronoiDiagram = show;
    update();
}

void CVTImageGLWidget::setShowDelaunay(bool show)
{
    showDelaunay = show;
    update();
}

void CVTImageGLWidget::resetView()
{
    // 图像CVT不需要重置视图
    update();
}

void CVTImageGLWidget::drawRandomPoints()
{
    if (canvasData.points.empty() || !showPoints) return;
    
    // 禁用深度测试，确保点在最上层
    glDisable(GL_DEPTH_TEST);
    
    // 使用持久化的点绘制资源
    pointVao.bind();
    pointProgram.bind();
    
    // 设置投影矩阵
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    pointProgram.setUniformValue("projection", projection);
    
    // 绘制点
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, currentPointCount);
    glDisable(GL_PROGRAM_POINT_SIZE);
    
    // 清理
    pointProgram.release();
    pointVao.release();
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}

void CVTImageGLWidget::drawVoronoiDiagram()
{
    if (voronoiCells.empty() || !showVoronoiDiagram) return;

    // 获取窗口尺寸和宽高比
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;

    // 设置投影矩阵
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }

    // 设置着色器 - 从文件加载
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/cvtwidget/shaders/cvt_voronoi.vert")) {
        qWarning() << "Voronoi vertex shader error:" << program.log();
    }
    
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/cvtwidget/shaders/cvt_voronoi.frag")) {
        qWarning() << "Voronoi fragment shader error:" << program.log();
    }
    
    if (!program.link()) {
        qWarning() << "Voronoi shader link error:" << program.log();
        return;
    }

    program.bind();
    program.setUniformValue("projection", projection);

    // 为每个Voronoi单元创建并绘制多边形
    for (const auto& cell : voronoiCells) {
        std::vector<float> vertices;
        vertices.reserve(cell.size() * 2);
        
        for (const auto& point : cell) {
            vertices.push_back(point.x());
            vertices.push_back(point.y());
        }

        // 创建临时VBO和VAO
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        
        vao.create();
        vao.bind();
        
        vbo.create();
        vbo.bind();
        vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
        
        // 设置顶点属性
        int posLoc = program.attributeLocation("aPos");
        if (posLoc != -1) {
            program.enableAttributeArray(posLoc);
            program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
        }
        
        // 绘制多边形边界
        glLineWidth(1.5f); // 增加线宽使其更明显
        glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(cell.size()));
        
        vao.release();
        vbo.release();
    }
    
    program.release();
}

std::vector<QVector2D> CVTImageGLWidget::clipVoronoiCellToRectangle(const std::vector<QVector2D>& cell, 
                                                           float left, float right, 
                                                           float bottom, float top)
{
    // 如果有图像，使用图像边界
    if (hasValidImage()) {
        QRectF bounds = getImageBounds();
        left = bounds.left();
        right = bounds.right();
        bottom = bounds.top();    // 注意：OpenGL坐标系中y轴向上
        top = bounds.bottom();    // 所以top > bottom
    }
    std::vector<QVector2D> clippedCell;
    
    // 检查单元是否需要裁剪
    bool needsClipping = false;
    for (const auto& pt : cell) {
        if (pt.x() < left || pt.x() > right || pt.y() < bottom || pt.y() > top) {
            needsClipping = true;
            break;
        }
    }
    
    if (!needsClipping) {
        return cell; // 如果单元完全在矩形内，直接返回
    }
    
    // 检查每条边是否与边界相交
    for (size_t i = 0; i < cell.size(); i++) {
        const QVector2D& start = cell[i];
        const QVector2D& end = cell[(i + 1) % cell.size()];
        
        // 起点在矩形内
        if (start.x() >= left && start.x() <= right && 
            start.y() >= bottom && start.y() <= top) {
            clippedCell.push_back(start);
        }
        
        // 检查边是否与边界相交
        std::vector<QVector2D> intersections;
        
        // 检查与左边界的交点
        if ((start.x() < left && end.x() > left) || 
            (start.x() > left && end.x() < left)) {
            float t = (left - start.x()) / (end.x() - start.x());
            float y = start.y() + t * (end.y() - start.y());
            if (y >= bottom && y <= top) {
                intersections.push_back(QVector2D(left, y));
            }
        }
        
        // 检查与右边界的交点
        if ((start.x() < right && end.x() > right) || 
            (start.x() > right && end.x() < right)) {
            float t = (right - start.x()) / (end.x() - start.x());
            float y = start.y() + t * (end.y() - start.y());
            if (y >= bottom && y <= top) {
                intersections.push_back(QVector2D(right, y));
            }
        }
        
        // 检查与下边界的交点
        if ((start.y() < bottom && end.y() > bottom) || 
            (start.y() > bottom && end.y() < bottom)) {
            float t = (bottom - start.y()) / (end.y() - start.y());
            float x = start.x() + t * (end.x() - start.x());
            if (x >= left && x <= right) {
                intersections.push_back(QVector2D(x, bottom));
            }
        }
        
        // 检查与上边界的交点
        if ((start.y() < top && end.y() > top) || 
            (start.y() > top && end.y() < top)) {
            float t = (top - start.y()) / (end.y() - start.y());
            float x = start.x() + t * (end.x() - start.x());
            if (x >= left && x <= right) {
                intersections.push_back(QVector2D(x, top));
            }
        }
        
        // 添加交点（按距离起点的远近排序）
        if (!intersections.empty()) {
            std::sort(intersections.begin(), intersections.end(), 
                [&](const QVector2D& a, const QVector2D& b) {
                    return (a - start).lengthSquared() < 
                           (b - start).lengthSquared();
                });
            
            for (const auto& pt : intersections) {
                clippedCell.push_back(pt);
            }
        }
    }
    
    // 确保裁剪后的多边形是闭合的
    if (!clippedCell.empty() && clippedCell.front() != clippedCell.back()) {
        clippedCell.push_back(clippedCell.front());
    }
    
    return clippedCell;
}

void CVTImageGLWidget::computeVoronoiDiagram()
{
    voronoiCells.clear();

    if (canvasData.points.empty()) return;

    // 获取当前边界
    QRectF bounds = getImageBounds();
    const float left = bounds.left();
    const float right = bounds.right();
    const float bottom = bounds.top();    // 注意：OpenGL坐标系中y轴向上
    const float top = bounds.bottom();    // 所以top > bottom

    // 直接使用已有的Delaunay三角剖分
    const Delaunay& dt = canvasData.dt;

    // 创建Voronoi图
    VoronoiDiagram vd(dt);

    // 遍历所有面（每个面对应一个采样点）
    for (auto fit = vd.faces_begin(); fit != vd.faces_end(); ++fit) {
        // 跳过无限面
        if (fit->is_unbounded()) continue;
        
        std::vector<QVector2D> cell;
        
        // 获取面的边界循环
        Ccb_halfedge_circulator ec_start = fit->ccb();
        Ccb_halfedge_circulator ec = ec_start;
        
        do {
            if (ec->has_target()) {
                Point p = ec->target()->point();
                cell.push_back(QVector2D(static_cast<float>(p.x()), 
                                static_cast<float>(p.y())));
            }
        } while (++ec != ec_start);
        
        // 裁剪单元到矩形边界
        std::vector<QVector2D> clippedCell = clipVoronoiCellToRectangle(cell, left, right, bottom, top);
        voronoiCells.push_back(clippedCell);
    }

    update();
}

void CVTImageGLWidget::drawDelaunayTriangles()
{
    if (canvasData.dt.number_of_faces() == 0 || !showDelaunay) 
        return;
        
    // 获取窗口尺寸和宽高比
    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;

    // 设置投影矩阵
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }

    // 准备顶点数据 (仅使用原始点集)
    std::vector<float> vertices;
    for (const auto& p : canvasData.points) {
        vertices.push_back(static_cast<float>(p.x()));
        vertices.push_back(static_cast<float>(p.y()));
    }

    // 准备索引数据 - 使用GL_LINES模式绘制边线
    std::vector<unsigned int> indices;
    std::unordered_map<Point, unsigned int> pointToIndex;
    
    // 创建点坐标到索引的映射
    for (unsigned int i = 0; i < canvasData.points.size(); i++) {
        pointToIndex[canvasData.points[i]] = i;
    }
    
    // 获取当前边界点（四个角点）
    std::vector<Point> boundaryPoints;
    if (hasValidImage()) {
        QRectF bounds = getImageBounds();
        float left = bounds.left();
        float right = bounds.right();
        float top = bounds.top();     // 注意：OpenGL坐标系中，top是上边界（y值较大）
        float bottom = bounds.bottom(); // bottom是下边界（y值较小）
        
        boundaryPoints = {
            Point(left, bottom),  // 左下
            Point(right, bottom), // 右下
            Point(left, top),     // 左上
            Point(right, top)     // 右上
        };
    } else {
        // 默认正方形边界
        boundaryPoints = {
            Point(-1.0, -1.0),
            Point(1.0, -1.0),
            Point(-1.0, 1.0),
            Point(1.0, 1.0)
        };
    }
    
    // 改为遍历所有有限边
    for (auto eit = canvasData.dt.finite_edges_begin(); 
         eit != canvasData.dt.finite_edges_end(); ++eit) 
    {
        // 获取边的两个端点
        auto face = eit->first;
        int edgeIndex = eit->second;
        auto vh1 = face->vertex(face->cw(edgeIndex));
        auto vh2 = face->vertex(face->ccw(edgeIndex));
        
        Point p1 = vh1->point();
        Point p2 = vh2->point();
        
        // 检查是否与边界点相连
        bool isBoundaryEdge = false;
        for (const auto& boundaryPt : boundaryPoints) {
            // 使用CGAL的精确比较
            if (CGAL::compare_xy(p1, boundaryPt) == CGAL::EQUAL || 
                CGAL::compare_xy(p2, boundaryPt) == CGAL::EQUAL) {
                isBoundaryEdge = true;
                break;
            }
        }
        
        // 添加边的两个端点索引（排除与边界点相连的边）
        if (!isBoundaryEdge && 
            pointToIndex.find(p1) != pointToIndex.end() && 
            pointToIndex.find(p2) != pointToIndex.end()) 
        {
            indices.push_back(pointToIndex[p1]);
            indices.push_back(pointToIndex[p2]);
        }
    }

    // 设置着色器 - 从文件加载
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/cvtwidget/shaders/cvt_delaunay.vert")) {
        qWarning() << "Delaunay vertex shader error:" << program.log();
    }
    
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/cvtwidget/shaders/cvt_delaunay.frag")) {
        qWarning() << "Delaunay fragment shader error:" << program.log();
    }
    
    if (!program.link()) {
        qWarning() << "Delaunay shader link error:" << program.log();
        return;
    }

    program.bind();
    program.setUniformValue("projection", projection);

    // 创建临时VAO和VBO
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo(QOpenGLBuffer::IndexBuffer);
    
    vao.create();
    vao.bind();
    
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
    
    ebo.create();
    ebo.bind();
    ebo.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));
    
    // 设置顶点属性
    int posLoc = program.attributeLocation("aPos");
    if (posLoc != -1) {
        program.enableAttributeArray(posLoc);
        program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }
    
    // 绘制三角形边线
    glLineWidth(1.5f);
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
    
    vao.release();
    program.release();
}

void CVTImageGLWidget::performLloydRelaxation()
{
    if (voronoiCells.empty()) {
        computeVoronoiDiagram();
    }
    
    // 存储新点位置
    std::vector<Point> newPoints = canvasData.points;
    
    // 前4个点是矩形角点，不移动
    for (size_t i = 4; i < canvasData.points.size(); i++) {
        // 获取当前点的Voronoi单元
        const std::vector<QVector2D>& cell = voronoiCells[i-4];
        
        // 计算单元重心
        float centroidX = 0.0f;
        float centroidY = 0.0f;
        float area = 0.0f;
        
        // 使用鞋带公式计算重心
        int n = cell.size();
        for (int j = 0; j < n; j++) {
            const QVector2D& p1 = cell[j];
            const QVector2D& p2 = cell[(j+1) % n];
            
            float cross = p1.x() * p2.y() - p2.x() * p1.y();
            area += cross;
            centroidX += (p1.x() + p2.x()) * cross;
            centroidY += (p1.y() + p2.y()) * cross;
        }
        
        if (fabs(area) > 1e-7) {
            area *= 0.5f;
            centroidX /= (6.0f * area);
            centroidY /= (6.0f * area);
            
            // 更新点位置
            newPoints[i] = Point(centroidX, centroidY);
        }
    }
    
    // 更新点集
    canvasData.points = newPoints;
    
    // 重新计算Delaunay三角剖分
    canvasData.dt = Delaunay(canvasData.points.begin(), canvasData.points.end());
    
    // 准备点数据
    std::vector<float> points;
    points.reserve(canvasData.points.size() * 2);
    for (const auto& point : canvasData.points) {
        points.push_back(static_cast<float>(point.x()));
        points.push_back(static_cast<float>(point.y()));
    }
    
    // 更新点缓冲区
    makeCurrent();
    pointVao.bind();
    pointVbo.bind();
    pointVbo.allocate(points.data(), static_cast<int>(points.size() * sizeof(float)));
    pointVao.release();
    pointVbo.release();
    doneCurrent();
    
    // 重新计算Voronoi图
    computeVoronoiDiagram();
    
    update();
}

// cvt_imageglwidget.cpp
QRectF CVTImageGLWidget::getImageBounds() const
{
    if (!hasValidImage()) {
        // 没有图像时返回默认正方形范围
        return QRectF(-1.0f, -1.0f, 2.0f, 2.0f);
    }

    float screenWidth = width();
    float screenHeight = height();
    float aspect = screenWidth / screenHeight;
    float imgAspect = static_cast<float>(loadedImage.width()) / loadedImage.height();

    float drawWidth, drawHeight;
    
    if (imgAspect > aspect) {
        // 宽度充满
        drawWidth = 2.0f * aspect;
        drawHeight = drawWidth / imgAspect;
    } else {
        // 高度充满
        drawHeight = 2.0f;
        drawWidth = drawHeight * imgAspect;
    }

    return QRectF(-drawWidth/2, -drawHeight/2, drawWidth, drawHeight);
}

std::vector<Point> CVTImageGLWidget::getBoundaryPoints() const
{
    if (hasValidImage()) {
        QRectF bounds = getImageBounds();
        float left = bounds.left();
        float right = bounds.right();
        float top = bounds.top();     // 注意：OpenGL坐标系中，top是上边界（y值较大）
        float bottom = bounds.bottom(); // bottom是下边界（y值较小）
        
        return {
            Point(left, bottom),  // 左下
            Point(right, bottom), // 右下
            Point(left, top),     // 左上
            Point(right, top)     // 右上
        };
    } else {
        // 默认正方形边界
        return {
            Point(-1.0, -1.0),
            Point(1.0, -1.0),
            Point(-1.0, 1.0),
            Point(1.0, 1.0)
        };
    }
}
