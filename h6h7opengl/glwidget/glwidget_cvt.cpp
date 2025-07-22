#include "glwidget.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>
#include <algorithm>
#include <cmath>

// CGAL 类型定义
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> DelaunayTriangulation;
typedef CGAL::Delaunay_triangulation_adaptation_traits_2<DelaunayTriangulation> AT;
typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2<DelaunayTriangulation> AP;
typedef CGAL::Voronoi_diagram_2<DelaunayTriangulation, AT, AP> VoronoiDiagram;
typedef K::Point_2 Point_2;
typedef VoronoiDiagram::Locate_result Locate_result;
typedef VoronoiDiagram::Face_handle Face_handle;
typedef VoronoiDiagram::Ccb_halfedge_circulator Ccb_halfedge_circulator;

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

void GLWidget::drawVoronoiDiagram()
{
    if (voronoiCells.empty()) return;

    // 设置简单的着色器
    QOpenGLShaderProgram program;
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4(aPos, 0.0, 1.0);\n"
        "}");
    
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.0, 0.0, 1.0, 1.0); // 蓝色线条\n"
        "}");
    
    if (!program.link()) {
        qWarning() << "Voronoi shader link error:" << program.log();
        return;
    }

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
        glLineWidth(1.5f);
        glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(cell.size()));
        
        vao.release();
        vbo.release();
    }
    
    program.release();
}

void GLWidget::computeVoronoiDiagram()
{
    voronoiCells.clear(); // 清空之前的Voronoi单元

    if (randomPoints.empty()) return;

    // 添加矩形的四个角点
    std::vector<Point_2> points;
    points.reserve(randomPoints.size() + 4);
    
    // 添加随机点
    for (const auto& p : randomPoints) {
        points.push_back(Point_2(p.x(), p.y()));
    }
    
    // 添加矩形边界点（按顺序：左下、右下、右上、左上）
    points.push_back(Point_2(-1.0, -1.0)); // 左下
    points.push_back(Point_2(1.0, -1.0));  // 右下
    points.push_back(Point_2(1.0, 1.0));   // 右上
    points.push_back(Point_2(-1.0, 1.0));  // 左上

    // 创建Delaunay三角剖分
    DelaunayTriangulation dt;
    dt.insert(points.begin(), points.end());

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
                Point_2 p = ec->target()->point();
                cell.push_back(QVector2D(p.x(), p.y()));
            }
        } while (++ec != ec_start);
        
        voronoiCells.push_back(cell);
    }

    update();
}

void GLWidget::performLloydRelaxation() {

}

void GLWidget::drawCVTBackground()
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
    
    // 创建模型矩阵
    QMatrix4x4 model;
    
    // 使用简单的着色器绘制白色背景
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

    // 在背景上绘制随机点
    if (!randomPoints.empty()) {
        // 绘制Voronoi图
        drawRandomPoints();    
        drawVoronoiDiagram();

    }
}