#include "glwidget.h"
// CVT 实现
void GLWidget::generateRandomPoints() {

}

void GLWidget::computeDelaunayTriangulation() {

}

void GLWidget::computeVoronoiDiagram() {

}

void GLWidget::performLloydRelaxation() {

}

void GLWidget::drawCVTBackground()
{
    // 设置正交投影
    QMatrix4x4 projection;
    projection.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    
    // 计算正方形大小（基于屏幕较短边）
    float screenWidth = width();
    float screenHeight = height();
    float squareSize = qMin(screenWidth, screenHeight) / qMax(screenWidth, screenHeight);
    
    // 创建模型矩阵
    QMatrix4x4 model;
    model.scale(squareSize, squareSize, 1.0f); // 保持正方形
    
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
    
    // 正方形顶点数据
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
}