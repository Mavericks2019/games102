#include "glwidget.h"

void GLWidget::generateRandomPoints(int count)
{
    randomPoints.clear();
    randomPoints.reserve(count);
    
    // 在 [-0.9, 0.9] 范围内生成随机点
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < count; i++) {
        float x = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        float y = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
        randomPoints.push_back(QVector2D(x, y));
    }
    
    currentPointCount = count;
    
    // 准备点数据
    std::vector<float> points;
    points.reserve(randomPoints.size() * 2);
    for (const auto& point : randomPoints) {
        points.push_back(point.x());
        points.push_back(point.y());
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
    
    doneCurrent();
    
    update();
}

void GLWidget::drawRandomPoints()
{
    if (randomPoints.empty()) return;
    
    // 禁用深度测试，确保点在最上层
    glDisable(GL_DEPTH_TEST);
    
    // 使用持久化的点绘制资源
    pointVao.bind();
    pointProgram.bind();
    
    // 设置投影矩阵（与背景相同）
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

void GLWidget::computeDelaunayTriangulation() {

}

void GLWidget::computeVoronoiDiagram() {

}

void GLWidget::performLloydRelaxation() {

}

void GLWidget::drawCVTBackground()
{
    // 获取窗口尺寸
    float screenWidth = width();
    float screenHeight = height();
    
    // 计算窗口宽高比
    float aspect = screenWidth / screenHeight;

    // 禁用深度测试
    //glDisable(GL_DEPTH_TEST);
    // 设置正交投影，根据宽高比调整
    QMatrix4x4 projection;
    if (aspect > 1.0f) {
        // 宽大于高
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        // 高大于宽
        projection.ortho(-1.0f, 1.0f, -1.0f/aspect, 1.0f/aspect, -1.0f, 1.0f);
    }
    
    // 创建模型矩阵 - 不需要缩放
    QMatrix4x4 model;
    
    // 使用简单的着色器
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * model * vec4(aPos, 1.0);\n"
        "}");
    
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // 白色\n"
        "}");
    
    if (!program.link()) {
        qWarning() << "Background shader link error:" << program.log();
        return;
    }
    
    // 正方形顶点数据（从-1到1的正方形）
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    
    // 索引数据
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
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
    program.bind();
    int posLoc = program.attributeLocation("aPos");
    if (posLoc != -1) {
        program.enableAttributeArray(posLoc);
        program.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    // 设置变换矩阵
    program.setUniformValue("model", model);
    program.setUniformValue("projection", projection);
    
    // 绘制正方形
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // 清理
    vao.release();
    program.release();

    // 在背景上绘制随机点
    if (!randomPoints.empty()) {
        drawRandomPoints();
    }
    
    // 重新启用深度测试
    glEnable(GL_DEPTH_TEST);
}